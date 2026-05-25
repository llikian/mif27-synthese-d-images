/***************************************************************************************************
 * @file  DrawImage.cpp
 * @brief Implementation of the DrawImage class
 **************************************************************************************************/

#include "DrawImage.hpp"

#include "draw.h"
#include "glcore.h"

DrawImage::DrawImage(std::size_t size, const vec3& clear_color) : size(size), data(new vec3[size * size]) {
    glGenTextures(1, &texture_id);
    clear(clear_color);
}

DrawImage::~DrawImage() {
    delete[] data;
    glDeleteTextures(1, &texture_id);
}

void DrawImage::set_as_default_texture() const {
    default_texture(0, texture_id);
}

void DrawImage::update_texture_data() const {
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, size, size, 0, GL_RGB, GL_FLOAT, data);
    glGenerateMipmap(GL_TEXTURE_2D);
}

void DrawImage::clear(const vec3& clear_color) {
    for(std::size_t i = 0; i < size * size; ++i) { data[i] = clear_color; }
    update_texture_data();
}

void DrawImage::draw_triangle(const vec3& draw_color, vec2 A, vec2 B, vec2 C) {
    static auto triangle_edge = [](const vec2& a, const vec2& b, const vec2& p) -> float {
        return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
    };

    A.x *= size;
    A.y *= size;
    B.x *= size;
    B.y *= size;
    C.x *= size;
    C.y *= size;

    // Triangle
    std::size_t min_x = std::min({ A.x, B.x, C.x });
    std::size_t max_x = std::max({ A.x, B.x, C.x });
    std::size_t min_y = std::min({ A.y, B.y, C.y });
    std::size_t max_y = std::max({ A.y, B.y, C.y });

    for(std::size_t y = min_y; y <= max_y; ++y) {
        for(std::size_t x = min_x; x <= max_x; ++x) {
            vec2 P(x + 0.5f, y + 0.5f);

            float w0 = triangle_edge(B, C, P);
            float w1 = triangle_edge(C, A, P);
            float w2 = triangle_edge(A, B, P);

            if(w0 >= 0 && w1 >= 0 && w2 >= 0) { data[y * size + x] = draw_color; }
        }
    }

    update_texture_data();
}

void DrawImage::draw_circle(const vec3& draw_color, vec2 center, float radius) {
    center.x *= size;
    center.y *= size;

    // Triangle
    std::size_t min_x = std::max(0.0f, center.x - radius);
    std::size_t max_x = std::min(static_cast<float>(size - 1), center.x + radius);
    std::size_t min_y = std::max(0.0f, center.y - radius);
    std::size_t max_y = std::min(static_cast<float>(size - 1), center.y + radius);

    for(std::size_t y = min_y; y <= max_y; ++y) {
        for(std::size_t x = min_x; x <= max_x; ++x) {
            float dx = x + 0.5f - center.x;
            float dy = y + 0.5f - center.y;

            if(dx * dx + dy * dy <= radius * radius) { data[y * size + x] = draw_color; }
        }
    }

    update_texture_data();
}
