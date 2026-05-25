/***************************************************************************************************
 * @file  DrawImage.hpp
 * @brief Declaration of the DrawImage class
 **************************************************************************************************/

#pragma once

#include "color.h"
#include "vec.h"

/**
 * @class DrawImage
 * @brief
 */
class DrawImage {
public:
    DrawImage(std::size_t size, const vec3& clear_color);
    ~DrawImage();

    void set_as_default_texture() const;
    void update_texture_data() const;

    void clear(const vec3& clear_color);

    void draw_triangle(const vec3& draw_color, vec2 A, vec2 B, vec2 C);
    void draw_circle(const vec3& draw_color, vec2 center, float radius);

private:
    std::size_t size;
    vec3* data;
    unsigned int texture_id;
};

