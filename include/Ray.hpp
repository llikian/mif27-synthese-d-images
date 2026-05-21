/***************************************************************************************************
 * @file  Ray.hpp
 * @brief Declaration of the Ray struct
 **************************************************************************************************/

#pragma once

#include <limits>
#include "vec.h"

static constexpr float infinity = std::numeric_limits<float>::infinity();
static constexpr float epsilon = std::numeric_limits<float>::epsilon();

inline Point vec4_to_point(const vec4& v) {
    return Point(v.x / v.w, v.y / v.w, v.z / v.w);
}

/**
 * @struct Ray
 * @brief
 */
struct Ray {
    explicit Ray(const vec3& origin) : origin(origin), direction(0.0f, 1.0f, 0.0f) {}

    Ray(const vec3& origin, const vec3& direction) : origin(origin), direction(normalize(direction)) {}

    Ray(const vec4& start, const vec4& end)
        : origin(vec4_to_point(start)),
          direction(normalize(vec4_to_point(end) - origin)) {}

    vec3 get_point(float distance) const;

    float intersect_triangle(const vec3& A, const vec3& B, const vec3& C) const;

    vec3 origin;
    vec3 direction;
};
