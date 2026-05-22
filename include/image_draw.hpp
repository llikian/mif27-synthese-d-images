/***************************************************************************************************
 * @file  image_draw.hpp
 * @brief Declaration of image_draw
 **************************************************************************************************/

#pragma once

#include "vec.h"

#define TEX_SIZE 2048ull

void clear_draw_texture(vec3* image, unsigned int texture, const vec3& clear_color);
float triangle_edge(vec2 a, vec2 b, vec2 p);
void draw_triangle(vec3* image, unsigned int texture, const vec3& draw_color, vec2 A, vec2 B, vec2 C);
void draw_circle(vec3* image, unsigned int texture, const vec3& draw_color, vec2 center, float radius);
