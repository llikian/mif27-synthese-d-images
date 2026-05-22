/***************************************************************************************************
 * @file  image_draw.cpp
 * @brief Implementation of image_draw
 **************************************************************************************************/

#include "image_draw.hpp"

#include "glcore.h"

void clear_draw_texture(vec3* image, unsigned int texture, const vec3& clear_color) {
    for(std::size_t i = 0; i < TEX_SIZE * TEX_SIZE; ++i) { image[i] = clear_color; }
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, TEX_SIZE, TEX_SIZE, 0, GL_RGB, GL_FLOAT, image);
    glGenerateMipmap(GL_TEXTURE_2D);
}

float triangle_edge(vec2 a, vec2 b, vec2 p) {
    return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
}

void draw_triangle(vec3* image, unsigned int texture, const vec3& draw_color, vec2 A, vec2 B, vec2 C) {
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

            float w0 = triangle_edge(B, C, p);
            float w1 = triangle_edge(C, A, p);
            float w2 = triangle_edge(A, B, p);

            if(w0 >= 0 && w1 >= 0 && w2 >= 0) { image[y * TEX_SIZE + x] = draw_color; }
        }
    }

    // Update texture data
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, TEX_SIZE, TEX_SIZE, 0, GL_RGB, GL_FLOAT, image);
    glGenerateMipmap(GL_TEXTURE_2D);
}

void draw_circle(vec3* image, unsigned int texture, const vec3& draw_color, vec2 center, float radius) {
    center.x *= TEX_SIZE;
    center.y *= TEX_SIZE;

    // Triangle
    std::size_t min_x = std::max(0.0f, center.x - radius);
    std::size_t max_x = std::min(static_cast<float>(TEX_SIZE - 1), center.x + radius);
    std::size_t min_y = std::max(0.0f, center.y - radius);
    std::size_t max_y = std::min(static_cast<float>(TEX_SIZE - 1), center.y + radius);

    for(std::size_t y = min_y; y <= max_y; ++y) {
        for(std::size_t x = min_x; x <= max_x; ++x) {
            float dx = x + 0.5f - center.x;
            float dy = y + 0.5f - center.y;

            if(dx * dx + dy * dy <= radius * radius) { image[y * TEX_SIZE + x] = draw_color; }
        }
    }

    // Update texture data
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, TEX_SIZE, TEX_SIZE, 0, GL_RGB, GL_FLOAT, image);
    glGenerateMipmap(GL_TEXTURE_2D);
}
