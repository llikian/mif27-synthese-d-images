/***************************************************************************************************
 * @file  Application.cpp
 * @brief Implementation of the Application class
 **************************************************************************************************/

#include "Application.hpp"

#include "buffers.h"
#include "constants.hpp"
#include "draw.h"
#include "glcore.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include "imgui_internal.h"
#include "lscm.hpp"
#include "Ray.hpp"
#include "texture.h"
#include "tutte.hpp"

bool is_mouse_hovering_imgui() {
    ImGuiContext* imgui_context = ImGui::GetCurrentContext();
    return imgui_context->HoveredWindow != nullptr &&
           (imgui_context->HoveredWindow->Flags & ImGuiWindowFlags_NoMouseInputs) == 0;
}

vec2 get_mouse_pos() {
    int mx;
    int my;
    SDL_GetMouseState(&mx, &my);

    return vec2(mx, my);
}

Application::Application(int win_width, int win_height)
    : win_width(win_width),
      win_height(win_height),
      window(create_window(win_width, win_height)),
      context(create_context(window)),
      should_stop(false),

      camera(vec3(0.0f, 2.0f, 10.0f), PI_F / 4.0f, static_cast<float>(win_width) / win_height, 0.1f, 1000.0f),

      time(0.0f),
      delta(0.0f),

      is_mouse_button_down(false),

      wireframe(false),

      draw_color_left(1.0f, 0.0f, 0.0f),
      draw_color_right(1.0f, 1.0f, 1.0f),
      draw_color(draw_color_left),
      draw_radius(5.0f),
      clear_color(draw_color_right),
      image_tutte(1024, clear_color),
      image_lscm(1024, clear_color),
      checker_texture(read_texture(0, "data/assets/blender_checker.png")),

      current_view(VIEW_SUZANNE_LSCM_UVS) {
    // ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init("#version 130");

    // OpenGL
    glViewport(0, 0, win_width, win_height);
    glClearColor(71 / 255.0f, 142 / 255.0f, 95 / 255.0f, 1.0f);
    glClearDepth(1);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);

    // Repeatable Keys
    repeatable_keys[SDLK_z] = false;
    repeatable_keys[SDLK_q] = false;
    repeatable_keys[SDLK_s] = false;
    repeatable_keys[SDLK_d] = false;
    repeatable_keys[SDLK_SPACE] = false;
    repeatable_keys[SDLK_c] = false;
    repeatable_keys[SDLK_LEFT] = false;
    repeatable_keys[SDLK_RIGHT] = false;
    repeatable_keys[SDLK_DOWN] = false;
    repeatable_keys[SDLK_UP] = false;
}

Application::~Application() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    release_buffers(vao);
    release_buffers(vao_tutte);
    release_buffers(vao_tutte_as_tex);
    release_buffers(vao_lscm);
    release_buffers(vao_lscm_as_tex);

    release_context(context);
    release_window(window);
}

void Application::init() {
    if(!read_meshio_data("data/assets/suzanne_uvsplit.obj", mesh_data)) {
        throw std::runtime_error("Couldn't read mesh");
    };

    tutte_uvs = tutte_uv_unwrapping(mesh_data);
    lscm_uvs = lscm_uv_unwrapping(mesh_data);

    vao = create_buffers(mesh_data.positions, mesh_data.indices, mesh_data.positions, mesh_data.normals);
    vao_tutte = create_buffers(mesh_data.positions, mesh_data.indices, tutte_uvs, mesh_data.normals);
    vao_tutte_as_tex = create_buffers(tutte_uvs, mesh_data.indices, tutte_uvs);
    vao_lscm = create_buffers(mesh_data.positions, mesh_data.indices, lscm_uvs, mesh_data.normals);
    vao_lscm_as_tex = create_buffers(lscm_uvs, mesh_data.indices, lscm_uvs);

    current_view = VIEW_SUZANNE_LSCM_UVS;
    image_lscm.set_as_default_texture();
}

void Application::run() {
    init();

    while(!should_stop) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        handle_events();
        handle_repeatable_keys();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        float new_time = SDL_GetTicks() / 1000.0f;
        delta = new_time - time;
        time = new_time;

        view = camera.get_view_matrix();
        projection = camera.get_projection_matrix();

        if(is_mouse_button_down && !is_mouse_hovering_imgui()) { handle_ray_cast(); }

        draw();
        draw_imgui_window();

        ImGui::Render();
        glViewport(0, 0, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
}

void Application::handle_events() {
    SDL_Event event;

    while(SDL_PollEvent(&event) != 0) {
        ImGui_ImplSDL2_ProcessEvent(&event);

        switch(event.type) {
            case SDL_QUIT: should_stop = true; break;
            case SDL_KEYDOWN:
                if(repeatable_keys.contains(event.key.keysym.sym)) { repeatable_keys[event.key.keysym.sym] = true; }

                switch(event.key.keysym.sym) {
                    case SDLK_ESCAPE: should_stop = true; break;
                    case SDLK_w:
                        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_FILL : GL_LINE);
                        wireframe = !wireframe;
                        break;
                    case SDLK_0:
                        current_view = VIEW_SUZANNE_DEFAULT_UVS;
                        default_texture(0, checker_texture);
                        break;
                    case SDLK_1:
                        current_view = VIEW_SUZANNE_TUTTE_UVS;
                        image_tutte.set_as_default_texture();
                        break;
                    case SDLK_2:
                        current_view = VIEW_TUTTE_UVS;
                        image_tutte.set_as_default_texture();
                        break;
                    case SDLK_3:
                        current_view = VIEW_SUZANNE_LSCM_UVS;
                        image_lscm.set_as_default_texture();
                        break;
                    case SDLK_4:
                        current_view = VIEW_LSCM_UVS;
                        image_lscm.set_as_default_texture();
                        break;
                    default: break;
                }
                break;
            case SDL_KEYUP:
                if(repeatable_keys.contains(event.key.keysym.sym)) { repeatable_keys[event.key.keysym.sym] = false; }
                break;
            case SDL_MOUSEBUTTONDOWN:
                if(event.button.button == SDL_BUTTON_LEFT) {
                    is_mouse_button_down = true;
                    draw_color = draw_color_left;
                } else if(event.button.button == SDL_BUTTON_RIGHT) {
                    is_mouse_button_down = true;
                    draw_color = draw_color_right;
                }
                break;
            case SDL_MOUSEBUTTONUP: is_mouse_button_down = false; break;
            case SDL_WINDOWEVENT:
                if(event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    win_width = event.window.data1;
                    win_height = event.window.data2;
                    camera.update_projection_matrix(static_cast<float>(win_width) / win_height);
                }
                break;
            default: break;
        }
    }
}

void Application::handle_repeatable_keys() {
    for(const auto& [key, is_key_down] : repeatable_keys) {
        constexpr float view_speed = 0.2f;
        if(is_key_down) {
            switch(key) {
                case SDLK_z:     camera.move_around(MovementDirection::FORWARD, delta); break;
                case SDLK_q:     camera.move_around(MovementDirection::LEFT, delta); break;
                case SDLK_s:     camera.move_around(MovementDirection::BACKWARD, delta); break;
                case SDLK_d:     camera.move_around(MovementDirection::RIGHT, delta); break;
                case SDLK_SPACE: camera.move_around(MovementDirection::UPWARD, delta); break;
                case SDLK_c:     camera.move_around(MovementDirection::DOWNWARD, delta); break;
                case SDLK_LEFT:  camera.look_around(0.0f, -view_speed); break;
                case SDLK_RIGHT: camera.look_around(0.0f, view_speed); break;
                case SDLK_UP:    camera.look_around(-view_speed, 0.0f); break;
                case SDLK_DOWN:  camera.look_around(view_speed, 0.0f); break;
                default:         break;
            }
        }
    }
}

void Application::handle_ray_cast() {
    std::vector<Point>* positions = nullptr;
    std::vector<Point>* uvs = nullptr;
    DrawImage* image = nullptr;

    switch(current_view) {
        case VIEW_SUZANNE_TUTTE_UVS:
            positions = &mesh_data.positions;
            uvs = &tutte_uvs;
            image = &image_tutte;
            break;
        case VIEW_TUTTE_UVS:
            positions = &tutte_uvs;
            uvs = &tutte_uvs;
            image = &image_tutte;
            break;
        case VIEW_SUZANNE_LSCM_UVS:
            positions = &mesh_data.positions;
            uvs = &lscm_uvs;
            image = &image_lscm;
            break;
        case VIEW_LSCM_UVS:
            positions = &lscm_uvs;
            uvs = &lscm_uvs;
            image = &image_lscm;
            break;
        default: break;
    }

    if(uvs != nullptr) {
        vec2 mouse_pos = get_mouse_pos();
        vec2 window_res(win_width, win_height);

        vec2 normalized_mouse_pos(-1.0f + 2.0f * mouse_pos.x / window_res.x, 1.0f - 2.0f * mouse_pos.y / window_res.y);

        Transform vp_inverse = (projection * view).inverse();

        Ray ray(vp_inverse(vec4(normalized_mouse_pos, -1.0f, 1.0f)),
                vp_inverse(vec4(normalized_mouse_pos, 1.0f, 1.0f)));

        bool intersected = false;
        std::size_t triangle_id = 0;
        float distance = 0.0f;

        for(std::size_t i = 0; i + 2 < mesh_data.indices.size(); i += 3) {
            float dist = ray.intersect_triangle(model(positions->at(mesh_data.indices[i])),
                                                model(positions->at(mesh_data.indices[i + 1])),
                                                model(positions->at(mesh_data.indices[i + 2])));

            if(dist > 0.0f) {
                if(dist < distance || !intersected) {
                    distance = dist;
                    triangle_id = i;
                }
                intersected = true;
            }
        }

        if(intersected) {
            std::size_t index0 = mesh_data.indices[triangle_id];
            std::size_t index1 = mesh_data.indices[triangle_id + 1];
            std::size_t index2 = mesh_data.indices[triangle_id + 2];

            vec2 A(uvs->at(index0).x, uvs->at(index0).y);
            vec2 B(uvs->at(index1).x, uvs->at(index1).y);
            vec2 C(uvs->at(index2).x, uvs->at(index2).y);

            vec3 barycentric_coords = ray.get_barycentric_coords_in_triangle(model(positions->at(index0)),
                                                                             model(positions->at(index1)),
                                                                             model(positions->at(index2)));
            vec2 P(A.x * barycentric_coords.x + B.x * barycentric_coords.y + C.x * barycentric_coords.z,
                   A.y * barycentric_coords.x + B.y * barycentric_coords.y + C.y * barycentric_coords.z);

            // draw_triangle(image, texture, draw_color, A, B, C);
            image->draw_circle(draw_color, P, draw_radius);
        }
    }
}

void Application::draw() {
    std::size_t indices_count = mesh_data.indices.size();
    switch(current_view) {
        case VIEW_SUZANNE_DEFAULT_UVS: ::draw(vao, GL_TRIANGLES, indices_count, model, view, projection); break;
        case VIEW_SUZANNE_TUTTE_UVS:   ::draw(vao_tutte, GL_TRIANGLES, indices_count, model, view, projection); break;
        case VIEW_TUTTE_UVS:           ::draw(vao_tutte_as_tex, GL_TRIANGLES, indices_count, model, view, projection); break;
        case VIEW_SUZANNE_LSCM_UVS:    ::draw(vao_lscm, GL_TRIANGLES, indices_count, model, view, projection); break;
        case VIEW_LSCM_UVS:            ::draw(vao_lscm_as_tex, GL_TRIANGLES, indices_count, model, view, projection); break;
        default:                       break;
    }
}

void Application::draw_imgui_window() {
    ImGui::Begin("Debug");

    switch(current_view) {
        case VIEW_SUZANNE_DEFAULT_UVS: ImGui::Text("View: Suzanne - Default UVs"); break;
        case VIEW_SUZANNE_TUTTE_UVS:   ImGui::Text("View: Suzanne - Tutte UVs"); break;
        case VIEW_TUTTE_UVS:           ImGui::Text("View: Tutte UVs"); break;
        case VIEW_SUZANNE_LSCM_UVS:    ImGui::Text("View: Suzanne - LSCM UVs"); break;
        case VIEW_LSCM_UVS:            ImGui::Text("View: LSCM UVs"); break;
        default:                       break;
    }

    ImGui::ColorEdit3("Clear Color", &clear_color.x);
    if(ImGui::Button("Clear Image")) {
        if(current_view == VIEW_SUZANNE_TUTTE_UVS || current_view == VIEW_TUTTE_UVS) {
            image_tutte.clear(clear_color);
        } else if(current_view == VIEW_SUZANNE_LSCM_UVS || current_view == VIEW_LSCM_UVS) {
            image_lscm.clear(clear_color);
        }
    }
    ImGui::ColorEdit3("Draw Color Left", &draw_color_left.x);
    ImGui::ColorEdit3("Draw Color Right", &draw_color_right.x);
    ImGui::SliderFloat("Draw Radius", &draw_radius, 1.0f, 100.0f);

    ImGui::End();
}
