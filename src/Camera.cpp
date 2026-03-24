/***************************************************************************************************
 * @file  Camera.cpp
 * @brief Implementation of the Camera class
 **************************************************************************************************/

#include "Camera.hpp"

#include <cmath>
#include <numbers>

static inline Transform perspective(float fov, float aspect, float near, float far) {
    return Transform(1.0f / (aspect * std::tan(0.5f * fov)),
                     0.0f,
                     0.0f,
                     0.0f,
                     0.0f,
                     1.0f / std::tan(0.5f * fov),
                     0.0f,
                     0.0f,
                     0.0f,
                     0.0f,
                     -(far + near) / (far - near),
                     -(2.0f * far * near) / (far - near),
                     0.0f,
                     0.0f,
                     -1.0f,
                     0.0f);
}

Camera::Camera(const vec3& position, float fov, float aspect_ratio, float near_distance, float far_distance)
    : sensitivity(0.1f),
      movement_speed(5.0f),
      position(position),
      pitch(0.0f),
      yaw(-PIf / 2.0f),
      fov(fov),
      near_distance(near_distance),
      far_distance(far_distance),
      view_matrix(1.0f),
      projection_matrix(perspective(fov, aspect_ratio, near_distance, far_distance)) {
    update_vectors_and_view_matrix();
}

vec3 Camera::get_position() const {
    return position;
}

vec3 Camera::get_direction() const {
    return direction;
}

vec3 Camera::get_right_vector() const {
    return right;
}

vec3 Camera::get_up_vector() const {
    return up;
}

float Camera::get_fov() const {
    return fov;
}

float Camera::get_near_distance() const {
    return near_distance;
}

float Camera::get_far_distance() const {
    return far_distance;
}

const Transform& Camera::get_view_matrix() const {
    return view_matrix;
}

const Transform& Camera::get_projection_matrix() const {
    return projection_matrix;
}

Transform Camera::get_view_projection_matrix() const {
    return Transform(projection_matrix.m[0][0] * view_matrix.m[0][0],
                     projection_matrix.m[0][0] * view_matrix.m[0][1],
                     projection_matrix.m[0][0] * view_matrix.m[0][2],
                     projection_matrix.m[0][0] * view_matrix.m[0][3],

                     projection_matrix.m[1][1] * view_matrix.m[1][0],
                     projection_matrix.m[1][1] * view_matrix.m[1][1],
                     projection_matrix.m[1][1] * view_matrix.m[1][2],
                     projection_matrix.m[1][1] * view_matrix.m[1][3],

                     projection_matrix.m[2][2] * view_matrix.m[2][0],
                     projection_matrix.m[2][2] * view_matrix.m[2][1],
                     projection_matrix.m[2][2] * view_matrix.m[2][2],
                     projection_matrix.m[2][2] * view_matrix.m[2][3] + projection_matrix.m[2][3],

                     -view_matrix.m[2][0],
                     -view_matrix.m[2][1],
                     -view_matrix.m[2][2],
                     -view_matrix.m[2][3]);
}

Transform Camera::get_rotation_matrix() const {
    return Transform(right.x, up.x, -direction.x, right.y, up.y, -direction.y, right.z, up.z, -direction.z);
}

Transform Camera::get_model_matrix() const {
    return Transform(right.x,
                     up.x,
                     -direction.x,
                     position.x,
                     right.y,
                     up.y,
                     -direction.y,
                     position.y,
                     right.z,
                     up.z,
                     -direction.z,
                     position.z,
                     0.0f,
                     0.0f,
                     0.0f,
                     1.0f);
}

Transform Camera::get_inverse_projection_matrix() const {
    return Transform(1.0f / projection_matrix.m[0][0],
                     0.0f,
                     0.0f,
                     0.0f,
                     0.0f,
                     1.0f / projection_matrix.m[1][1],
                     0.0f,
                     0.0f,
                     0.0f,
                     0.0f,
                     0.0f,
                     -1.0f,
                     0.0f,
                     0.0f,
                     1.0f / projection_matrix.m[2][3],
                     projection_matrix.m[2][2] / projection_matrix.m[2][3]);
}

Transform Camera::get_inverse_view_projection_matrix() const {
    return get_model_matrix() * get_inverse_projection_matrix();
}

void Camera::set_position(const vec3& position) {
    this->position = position;

    view_matrix.m[0][3] = -dot(position, right);
    view_matrix.m[1][3] = -dot(position, up);
    view_matrix.m[2][3] = dot(position, direction);
}

void Camera::look_around(float pitch_offset, float yaw_offset) {
    static const float MAX_TILT_ANGLE = radians(80.0f);

    pitch = std::clamp(pitch - sensitivity * radians(pitch_offset), -MAX_TILT_ANGLE, MAX_TILT_ANGLE);
    yaw += sensitivity * radians(yaw_offset);

    update_vectors_and_view_matrix();
}

void Camera::move_around(MovementDirection movement_direction, float delta) {
    Vector dir;
    switch(movement_direction) {
        case MovementDirection::FORWARD:  position += movement_speed * delta * direction; break;
        case MovementDirection::BACKWARD: position -= movement_speed * delta * direction; break;
        case MovementDirection::LEFT:     position -= movement_speed * delta * right; break;
        case MovementDirection::RIGHT:    position += movement_speed * delta * right; break;
        case MovementDirection::UPWARD:   position += movement_speed * delta * WORLD_UP; break;
        case MovementDirection::DOWNWARD: position -= movement_speed * delta * WORLD_UP; break;
        default:                          break;
    }

    view_matrix.m[0][3] = -dot(position, right);
    view_matrix.m[1][3] = -dot(position, up);
    view_matrix.m[2][3] = dot(position, direction);
}

void Camera::update_projection_matrix() {
    // TODO: fix this ig
    // projection_matrix.m[0][0] = 1.0f / (Window::get_aspect_ratio() * std::tan(0.5f * fov));
}

void Camera::look_at_point(const vec3& target) {
    vec3 dir = normalize(target - position);
    pitch = std::asin(dir.y);
    yaw = std::atan2(dir.z, dir.x);
    update_vectors_and_view_matrix();
}

void Camera::update_vectors_and_view_matrix() {
    direction.x = std::cos(pitch) * std::cos(yaw);
    direction.y = std::sin(pitch);
    direction.z = std::cos(pitch) * std::sin(yaw);

    right = normalize(cross(direction, WORLD_UP));
    up = normalize(cross(right, direction));

    view_matrix.m[0][0] = right.x;
    view_matrix.m[0][1] = right.y;
    view_matrix.m[0][2] = right.z;
    view_matrix.m[0][3] = -dot(position, right);

    view_matrix.m[1][0] = up.x;
    view_matrix.m[1][1] = up.y;
    view_matrix.m[1][2] = up.z;
    view_matrix.m[1][3] = -dot(position, up);

    view_matrix.m[2][0] = -direction.x;
    view_matrix.m[2][1] = -direction.y;
    view_matrix.m[2][2] = -direction.z;
    view_matrix.m[2][3] = dot(position, direction);
}
