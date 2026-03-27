/***************************************************************************************************
 * @file  main.cpp
 * @brief Contains the main program of the project
 **************************************************************************************************/

#include <eigen3/Eigen/Dense>
#include <eigen3/Eigen/Sparse>
#include <iostream>
#include <print>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include "buffers.h"
#include "Camera.hpp"
#include "draw.h"
#include "glcore.h"
#include "mesh_io.h"
#include "texture.h"
#include "window.h"

using Edge = std::pair<unsigned int, unsigned int>;

std::vector<unsigned int> find_seam(const std::vector<unsigned int>& indices) {
    std::set<Edge> edges;

    auto emplace_edge = [&](unsigned int A, unsigned int B) {
        auto ite = edges.find(Edge(B, A));
        if(ite == edges.end()) {
            edges.emplace(A, B);
        } else {
            edges.erase(ite);
        }
    };

    for(std::size_t i = 0; i + 2 < indices.size(); i += 3) {
        emplace_edge(indices[i], indices[i + 1]);
        emplace_edge(indices[i + 1], indices[i + 2]);
        emplace_edge(indices[i + 2], indices[i]);
    }

    std::unordered_map<unsigned int, unsigned int> edges_map;
    for(const Edge& edge : edges) { edges_map[edge.first] = edge.second; }

    std::vector<unsigned int> vertices;
    unsigned int start = edges.begin()->first;
    unsigned int index = start;
    do {
        vertices.push_back(index);
        index = edges_map[index];
    } while(index != start);

    return vertices;
}

int main() {
    try {
        Window window = create_window(1024, 576);
        Context context = create_context(window);

        Camera camera(vec3(0.0f, -2.0f, -10.0f), PIf / 4.0f, 1024.0f / 576.0f, 0.1f, 1000.0f);

        MeshIOData data;
        if(!read_meshio_data("data/assets/suzanne_uvsplit.obj", data)) {
            throw std::runtime_error("Couldn't read mesh");
        };
        unsigned int vao = create_buffers(data.positions, data.indices, data.positions, data.normals);
        // unsigned int texture = read_texture(0, "data/assets/tomato.png");
        unsigned int texture = read_texture(0, "data/assets/blender_checker.png");
        default_texture(0, texture);

        // etat openGL de base / par defaut
        glViewport(0, 0, 1024, 576);
        glClearColor(71 / 255.0f, 142 / 255.0f, 95 / 255.0f, 1.0f);
        glClearDepth(1);
        glDepthFunc(GL_LESS);
        glEnable(GL_DEPTH_TEST);

        Transform model;

        float time = 0.0f;
        float delta = 0.0f;

        std::vector<unsigned int> seam = find_seam(data.indices);
        std::set<unsigned int> seam_set(seam.begin(), seam.end());

        std::cout << "\n\n";
        std::cout << "Seam vertices count: " << seam.size() << '\n';
        std::cout << "\n\n";

        std::size_t vertex_count = data.positions.size();
        std::vector<Point> tex_coords(vertex_count);
        {
            std::size_t num = 0;
            for(unsigned int index : seam) {
                float angle = (2.0f * PIf * num) / seam.size();
                tex_coords[index] = Point(std::cos(angle), std::sin(angle), 0.0f);
                num++;
            }
        }

        std::vector<std::set<unsigned int>> neighbours(vertex_count);
        for(std::size_t i = 0; i + 2 < data.indices.size(); i += 3) {
            unsigned int index0 = data.indices[i];
            unsigned int index1 = data.indices[i + 1];
            unsigned int index2 = data.indices[i + 2];
            neighbours[index0].insert(index1);
            neighbours[index0].insert(index2);
            neighbours[index1].insert(index0);
            neighbours[index1].insert(index2);
            neighbours[index2].insert(index0);
            neighbours[index2].insert(index1);
        }

        std::unordered_map<unsigned int, unsigned int> row_indices;
        unsigned int row = 0;
        for(std::size_t i = 0; i < vertex_count; ++i) {
            if(!seam_set.contains(i)) {
                row_indices[i] = row;
                row++;
            }
        }
        unsigned int interior_count = row_indices.size();
        std::cout << "Interior: " << interior_count << '\n';

        std::vector<Eigen::Triplet<float>> coefficients;
        Eigen::VectorXf rhs_u(interior_count);
        Eigen::VectorXf rhs_v(interior_count);
        rhs_u.setZero();
        rhs_v.setZero();

        for(const auto& [vertex_index, row_index] : row_indices) {
            coefficients.emplace_back(row_index, row_index, -static_cast<float>(neighbours[vertex_index].size()));

            for(unsigned int neighbour_index : neighbours[vertex_index]) {
                if(seam_set.contains(neighbour_index)) {
                    rhs_u[row_index] -= tex_coords[neighbour_index].x;
                    rhs_v[row_index] -= tex_coords[neighbour_index].y;
                } else {
                    coefficients.emplace_back(row_index, row_indices[neighbour_index], 1.0f);
                }
            }
        }

        Eigen::SparseMatrix<float> matrix(interior_count, interior_count);
        matrix.setFromTriplets(coefficients.begin(), coefficients.end());
        Eigen::ConjugateGradient<Eigen::SparseMatrix<float>> solver;
        Eigen::VectorXf solution_u(interior_count);
        Eigen::VectorXf solution_v(interior_count);
        solver.compute(matrix);
        solution_u = solver.solve(rhs_u);
        solution_v = solver.solve(rhs_v);

        for(const auto& [vertex_index, row_index] : row_indices) {
            tex_coords[vertex_index] = Point(solution_u[row_index], solution_v[row_index], 0.0f);
        }

        unsigned int vao_with_texcoords = create_buffers(data.positions, data.indices, tex_coords, data.normals);
        unsigned int vao_tex = create_buffers(tex_coords, data.indices);

        const SDL_Keycode KEYS[] {
            SDLK_z, SDLK_q, SDLK_s, SDLK_d, SDLK_SPACE, SDLK_c, SDLK_LEFT, SDLK_RIGHT, SDLK_DOWN, SDLK_UP,
        };
        std::unordered_map<SDL_Keycode, bool> repeatable_keys;
        for(SDL_Keycode key : KEYS) { repeatable_keys[key] = false; }

        bool wireframe = false;

        // main loop
        bool should_stop = false;
        while(!should_stop) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            delta = time - (SDL_GetTicks() / 1000.0f);
            time = SDL_GetTicks() / 1000.0f;

            SDL_Event event;
            while(SDL_PollEvent(&event) != 0) {
                if(event.type == SDL_QUIT) {
                    should_stop = true;
                } else if(event.type == SDL_KEYDOWN) {
                    if(repeatable_keys.contains(event.key.keysym.sym)) { repeatable_keys[event.key.keysym.sym] = true; }

                    switch(event.key.keysym.sym) {
                        case SDLK_ESCAPE: should_stop = true; break;
                        case SDLK_w:
                            if(wireframe) {
                                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

                            } else {

                                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                            }
                            wireframe = !wireframe;
                            break;
                        default: break;
                    }
                } else if(event.type == SDL_KEYUP) {

                    if(repeatable_keys.contains(event.key.keysym.sym)) {
                        repeatable_keys[event.key.keysym.sym] = false;
                    }
                }
            }

            for(const auto& [key, is_key_down] : repeatable_keys) {
                constexpr float view_speed = 10.0f;
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
                        case SDLK_DOWN:  camera.look_around(view_speed, 0.0f); break;
                        case SDLK_UP:    camera.look_around(-view_speed, 0.0f); break;
                        default:         break;
                    }
                }
            }

            Transform view = camera.get_view_matrix();

            // draw(vao,
            //      GL_TRIANGLES,
            //      data.indices.size(),
            //      model,
            //      view,
            //      camera.get_projection_matrix());

            draw(vao_with_texcoords, GL_TRIANGLES, data.indices.size(), model, view, camera.get_projection_matrix());

            // draw(vao_tex,
            //      GL_TRIANGLES,
            //      data.indices.size(),
            //      model,
            //      view,
            //      camera.get_projection_matrix());

            SDL_GL_SwapWindow(window);
        }

        std::cout << "\n\n";

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
