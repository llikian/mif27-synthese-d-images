/***************************************************************************************************
 * @file  main.cpp
 * @brief Contains the main program of the project
 **************************************************************************************************/

#include <iostream>
#include <print>
#include <stdexcept>
#include <unordered_map>
#include "buffers.h"
#include "Camera.hpp"
#include "constants.hpp"
#include "draw.h"
#include "glcore.h"
#include "least_squares.hpp"
#include "mesh_io.h"
#include "Ray.hpp"
#include "texture.h"
#include "tutte.hpp"
#include "window.h"

#define TEX_SIZE 1024ull

float edge(vec2 a, vec2 b, vec2 p) {
    return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
}

int main() {
    try {
        int width = 1024;
        int height = 576;

        Window window = create_window(width, height);
        Context context = create_context(window);

        Camera camera(vec3(0.0f, -2.0f, -10.0f), PI_F / 4.0f, static_cast<float>(width) / height, 0.1f, 1000.0f);

        MeshIOData data;
        if(!read_meshio_data("data/assets/suzanne_uvsplit.obj", data)) {
            throw std::runtime_error("Couldn't read mesh");
        };
        unsigned int vao = create_buffers(data.positions, data.indices, data.positions, data.normals);

        // unsigned int texture = read_texture(0, "data/assets/tomato.png");
        unsigned int checker_texture = read_texture(0, "data/assets/blender_checker.png");

        vec3* image = new vec3[TEX_SIZE * TEX_SIZE];
        for(std::size_t i = 0; i < TEX_SIZE * TEX_SIZE; ++i) { image[i] = vec3(1.0f, 1.0f, 1.0f); }
        unsigned int texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, TEX_SIZE, TEX_SIZE, 0, GL_RGB, GL_FLOAT, image);
        glGenerateMipmap(GL_TEXTURE_2D);

        const auto draw_triangle = [texture, image](vec2 A, vec2 B, vec2 C) {
            A.x *= TEX_SIZE;
            A.y *= TEX_SIZE;
            B.x *= TEX_SIZE;
            B.y *= TEX_SIZE;
            C.x *= TEX_SIZE;
            C.y *= TEX_SIZE;

            // Triangle
            std::size_t min_x = std::min({ A.x, B.x, C.x });
            std::size_t max_x = std::max({ A.x, B.x, C.x });
            std::size_t min_y = std::min({ A.y, B.y, C.y });
            std::size_t max_y = std::max({ A.y, B.y, C.y });

            for(std::size_t y = min_y; y <= max_y; ++y) {
                for(std::size_t x = min_x; x <= max_x; ++x) {
                    vec2 p(x + 0.5f, y + 0.5f);

                    float w0 = edge(B, C, p);
                    float w1 = edge(C, A, p);
                    float w2 = edge(A, B, p);

                    if(w0 >= 0 && w1 >= 0 && w2 >= 0) { image[y * TEX_SIZE + x] = vec3(1.0f, 0.0f, 0.0f); }
                }
            }

            // Update texture data
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, TEX_SIZE, TEX_SIZE, 0, GL_RGB, GL_FLOAT, image);
            glGenerateMipmap(GL_TEXTURE_2D);
        };

        default_texture(0, texture);
        default_texture(0, checker_texture);

        // etat openGL de base / par defaut
        glViewport(0, 0, width, height);
        glClearColor(71 / 255.0f, 142 / 255.0f, 95 / 255.0f, 1.0f);
        glClearDepth(1);
        glDepthFunc(GL_LESS);
        glEnable(GL_DEPTH_TEST);

        Transform model = Identity();

        float time = 0.0f;
        float delta = 0.0f;

        std::vector<Point> tutte_uvs = tutte_uv_unwrapping(data);
        unsigned int vao_tutte = create_buffers(data.positions, data.indices, tutte_uvs, data.normals);
        unsigned int vao_tutte_as_tex = create_buffers(tutte_uvs, data.indices);

        std::vector<Point> least_squares_uvs = least_squares_uv_unwrapping(data);
        unsigned int vao_least_squares = create_buffers(data.positions, data.indices, least_squares_uvs, data.normals);
        unsigned int vao_least_squares_as_tex = create_buffers(least_squares_uvs, data.indices);

        const SDL_Keycode KEYS[] {
            SDLK_z, SDLK_q, SDLK_s, SDLK_d, SDLK_SPACE, SDLK_c, SDLK_LEFT, SDLK_RIGHT, SDLK_DOWN, SDLK_UP,
        };
        std::unordered_map<SDL_Keycode, bool> repeatable_keys;
        for(SDL_Keycode key : KEYS) { repeatable_keys[key] = false; }

        bool wireframe = false;
        int view_number = 3;

        // main loop
        bool should_stop = false;
        while(!should_stop) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            delta = time - (SDL_GetTicks() / 1000.0f);
            time = SDL_GetTicks() / 1000.0f;

            Transform view = camera.get_view_matrix();
            Transform proj = camera.get_projection_matrix();

            SDL_Event event;
            while(SDL_PollEvent(&event) != 0) {
                switch(event.type) {
                    case SDL_QUIT: should_stop = true; break;
                    case SDL_KEYDOWN:
                        if(repeatable_keys.contains(event.key.keysym.sym)) {
                            repeatable_keys[event.key.keysym.sym] = true;
                        }

                        switch(event.key.keysym.sym) {
                            case SDLK_ESCAPE: should_stop = true; break;
                            case SDLK_w:
                                if(wireframe) {
                                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                                } else {
                                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                                }
                                wireframe = !wireframe;
                                break;
                            case SDLK_0: view_number = 0; break;
                            case SDLK_1: view_number = 1; break;
                            case SDLK_2: view_number = 2; break;
                            case SDLK_3: view_number = 3; break;
                            case SDLK_4: view_number = 4; break;
                            default:     break;
                        }
                        break;
                    case SDL_KEYUP:
                        if(repeatable_keys.contains(event.key.keysym.sym)) {
                            repeatable_keys[event.key.keysym.sym] = false;
                        }
                        break;
                    case SDL_MOUSEBUTTONDOWN:
                        if(event.button.button == SDL_BUTTON_LEFT) {
                            vec2 mouse_pos(event.button.x, event.button.y);
                            vec2 window_res(width, height);

                            vec2 normalized_mouse_pos(-1.0f + 2.0f * mouse_pos.x / window_res.x,
                                                      1.0f - 2.0f * mouse_pos.y / window_res.y);

                            // Transform vp_inverse = camera.get_inverse_view_projection_matrix();
                            Transform vp_inverse = (proj * view).inverse();

                            Ray ray(vp_inverse(vec4(normalized_mouse_pos, -1.0f, 1.0f)),
                                    vp_inverse(vec4(normalized_mouse_pos, 1.0f, 1.0f)));

                            bool intersected = false;
                            std::size_t triangle_id = 0;
                            float distance = 0.0f;

                            for(std::size_t i = 0; i + 2 < data.indices.size(); i += 3) {
                                float dist = ray.intersect_triangle(model(data.positions[data.indices[i]]),
                                                                    model(data.positions[data.indices[i + 1]]),
                                                                    model(data.positions[data.indices[i + 2]]));

                                if(dist > 0.0f) {
                                    if(dist < distance || !intersected) {
                                        distance = dist;
                                        triangle_id = i;
                                    }
                                    intersected = true;
                                }
                            }

                            if(intersected) {
                                const std::vector<Point>& uvs = least_squares_uvs;
                                draw_triangle(
                                    vec2(uvs[data.indices[triangle_id]].x, uvs[data.indices[triangle_id]].y),
                                    vec2(uvs[data.indices[triangle_id + 1]].x, uvs[data.indices[triangle_id + 1]].y),
                                    vec2(uvs[data.indices[triangle_id + 2]].x, uvs[data.indices[triangle_id + 2]].y));
                            }

                            std::cout << '\n';
                        }
                        break;
                }
            }

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

            // 0 : Suzanne with default texcoords
            // 1 : Tutte on Suzanne
            // 2 : Tutte UVs
            // 3 : Least Squares on Suzanne
            // 4 : Least Squares UVs
            switch(view_number) {
                case 0:  draw(vao, GL_TRIANGLES, data.indices.size(), model, view, proj); break;
                case 1:  draw(vao_tutte, GL_TRIANGLES, data.indices.size(), model, view, proj); break;
                case 2:  draw(vao_tutte_as_tex, GL_TRIANGLES, data.indices.size(), model, view, proj); break;
                case 3:  draw(vao_least_squares, GL_TRIANGLES, data.indices.size(), model, view, proj); break;
                case 4:  draw(vao_least_squares_as_tex, GL_TRIANGLES, data.indices.size(), model, view, proj); break;
                default: break;
            }

            SDL_GL_SwapWindow(window);
        }

        std::cout << "\n\n";

        delete[] image;

        release_buffers(vao);
        release_context(context);
        release_window(window);
        glDeleteTextures(1, &texture);
    } catch(const std::exception& exception) {
        std::cerr << "ERROR : " << exception.what() << '\n';
        return -1;
    }

    return 0;
}
