/***************************************************************************************************
 * @file  tutte.cpp
 * @brief Implementation of tutte
 **************************************************************************************************/

#include "tutte.hpp"

#include <cmath>
#include <eigen3/Eigen/Dense>
#include <eigen3/Eigen/Sparse>
#include <set>

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

std::vector<Point> tutte_uv_unwrapping(const MeshIOData& data) {
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
            float angle = (2.0f * std::numbers::pi_v<float> * num) / seam.size();
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

    float lowest_uv = std::numeric_limits<float>::max();
    float highest_uv = std::numeric_limits<float>::lowest();
    for(auto [u, v, z] : tex_coords) {
        lowest_uv = std::min({ lowest_uv, u, v });
        highest_uv = std::max({ highest_uv, u, v });
    }

    for(auto& [u, v, z] : tex_coords) {
        u = (u - lowest_uv) / (highest_uv - lowest_uv);
        v = (v - lowest_uv) / (highest_uv - lowest_uv);
    }

    return tex_coords;
}
