/***************************************************************************************************
 * @file  Ray.cpp
 * @brief Implementation of the Ray class
 **************************************************************************************************/

#include "Ray.hpp"

vec3 Ray::get_point(float distance) const {
    return origin + distance * direction;
}

float Ray::intersect_triangle(const vec3& A, const vec3& B, const vec3& C) const {
    vec3 edge1 = B - A;
    vec3 edge2 = C - A;
    vec3 origin_cross_edge2 = cross(direction, edge2);

    float determinent = dot(edge1, origin_cross_edge2);
    if(std::abs(determinent) < epsilon) { return -infinity; }
    float determinent_inv = 1.0f / determinent;

    vec3 A_to_origin = origin - A;
    float u = determinent_inv * dot(A_to_origin, origin_cross_edge2);
    if(u < 0.0f || u > 1.0f) { return -infinity; }

    vec3 s_cross_edge1 = cross(A_to_origin, edge1);
    float v = determinent_inv * dot(direction, s_cross_edge1);
    if(v < 0.0f || u + v > 1.0f) { return -infinity; }

    float t = determinent_inv * dot(edge2, s_cross_edge1);
    return t > 0.0f ? t : -infinity;
}

vec3 Ray::get_barycentric_coords_in_triangle(const vec3& A, const vec3& B, const vec3& C) const {
    vec3 edge1 = B - A;
    vec3 edge2 = C - A;
    vec3 origin_cross_edge2 = cross(direction, edge2);

    float determinent = dot(edge1, origin_cross_edge2);
    float determinent_inv = 1.0f / determinent;

    vec3 A_to_origin = origin - A;
    vec3 s_cross_edge1 = cross(A_to_origin, edge1);

    float u = determinent_inv * dot(A_to_origin, origin_cross_edge2);
    float v = determinent_inv * dot(direction, s_cross_edge1);
    float t = determinent_inv * dot(edge2, s_cross_edge1);

    return vec3(1.0f - u - v, u, v);
}
