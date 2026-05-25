/***************************************************************************************************
 * @file  lscm.cpp
 * @brief Implementation of lscm
 **************************************************************************************************/

#include "lscm.hpp"
#include <cstdint>
#include <Eigen/IterativeLinearSolvers>
#include <Eigen/SparseCore>

std::vector<Point> lscm_uv_unwrapping(const MeshIOData& data) {
    std::size_t vertex_count = data.positions.size();
    std::vector<Point> tex_coords(vertex_count);

    std::pair<std::size_t, float> lowest_y(0, std::numeric_limits<float>::max());
    std::pair<std::size_t, float> highest_y(0, std::numeric_limits<float>::lowest());

    for(std::size_t i = 0; i < vertex_count; ++i) {
        if(data.positions[i].y < lowest_y.second) { lowest_y = { i, data.positions[i].y }; }
        if(data.positions[i].y > highest_y.second) { highest_y = { i, data.positions[i].y }; }
    }

    tex_coords[lowest_y.first] = Point(0.5f, 0.2f, 0.0f);
    tex_coords[highest_y.first] = Point(0.5f, 0.8f, 0.0f);

    const auto variable_index = [lowest_y, highest_y](std::size_t index) -> std::int64_t {
        if(index == lowest_y.first) { return -1; }
        if(index == highest_y.first) { return -2; }

        std::size_t result = index;
        if(result > lowest_y.first) { result -= 1; }
        if(result > highest_y.first) { result -= 1; }

        return result;
    };

    const auto subscript = [](const Point& p, std::size_t index) -> float { return *(&p.x + index % 3); };

    std::size_t system_rows = 2 * data.indices.size() / 3;

    std::vector<Eigen::Triplet<float>> coefficients;
    Eigen::VectorXf rhs(system_rows);
    rhs.setZero();

    const std::vector<Point>& pos = data.positions;
    for(std::size_t i = 0; i + 2 < data.indices.size(); i += 3) {
        std::size_t f = i / 3;

        // Local Basis
        Vector v01 = pos[data.indices[i + 1]] - pos[data.indices[i]];
        Vector v02 = pos[data.indices[i + 2]] - pos[data.indices[i]];

        Vector e0 = normalize(v01);
        Vector e2 = cross(e0, v02);
        Vector e1 = normalize(cross(e2, e0));

        // Twice the triangle area
        float dt = std::sqrt(length(cross(v01, v02)));

        // Local coordinates
        Point px(0.0f, dot(v01, e0), dot(v02, e0));
        Point py(0.0f, dot(v01, e1), dot(v02, e1));

        // Fill Matrices
        for(std::uint8_t j = 0; j < 3; ++j) {
            float dax = (subscript(py, j + 1) - subscript(py, j + 2)) / dt;
            float day = (subscript(px, j + 2) - subscript(px, j + 1)) / dt;

            std::int64_t index = variable_index(data.indices[i + j]);
            if(index < 0) {
                index = (index == -1) ? lowest_y.first : highest_y.first;
                float u = tex_coords[index].x;
                float v = tex_coords[index].y;

                rhs[2 * f] -= day * u + dax * v;
                rhs[2 * f + 1] += dax * u - day * v;
            } else {
                coefficients.emplace_back(2 * f, 2 * index, day);
                coefficients.emplace_back(2 * f, 2 * index + 1, dax);
                coefficients.emplace_back(2 * f + 1, 2 * index, -dax);
                coefficients.emplace_back(2 * f + 1, 2 * index + 1, day);
            }
        }
    }

    Eigen::SparseMatrix<float> matrix(system_rows, 2 * (vertex_count - 2));
    matrix.setFromTriplets(coefficients.begin(), coefficients.end());

    Eigen::LeastSquaresConjugateGradient<Eigen::SparseMatrix<float>> solver;
    solver.compute(matrix);
    Eigen::VectorXf solution = solver.solve(rhs);

    for(std::size_t i = 0; i < vertex_count; ++i) {
        std::int64_t index = variable_index(i);
        if(index >= 0) { tex_coords[i] = Point(solution[2 * index], solution[2 * index + 1], 0.0f); };
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
