/***************************************************************************************************
 * @file  Application.hpp
 * @brief Declaration of the Application class
 **************************************************************************************************/

#pragma once

#include <cstdint>
#include <unordered_map>
#include "Camera.hpp"
#include "DrawImage.hpp"
#include "mesh_io.h"
#include "vec.h"
#include "window.h"

enum ViewName : std::uint8_t {
    VIEW_SUZANNE_DEFAULT_UVS,
    VIEW_SUZANNE_TUTTE_UVS,
    VIEW_TUTTE_UVS,
    VIEW_SUZANNE_LSCM_UVS,
    VIEW_LSCM_UVS,

    VIEW_COUNT
};

bool is_mouse_hovering_imgui();
vec2 get_mouse_pos();

/**
 * @class Application
 * @brief
 */
class Application {
public:
    Application(int win_width, int win_height);
    ~Application();

    void init();
    void run();
    void handle_events();
    void handle_repeatable_keys();
    void handle_ray_cast();
    void draw();
    void draw_imgui_window();

private:
    // Engine
    int win_width;
    int win_height;
    Window window;
    Context context;
    bool should_stop;

    Camera camera;
    Transform model;
    Transform view;
    Transform projection;

    float time;
    float delta;

    std::unordered_map<SDL_Keycode, bool> repeatable_keys;
    bool is_mouse_button_down;

    bool wireframe;

    // Data
    MeshIOData mesh_data;

    std::vector<Point> tutte_uvs;
    std::vector<Point> lscm_uvs;

    unsigned int vao;
    unsigned int vao_tutte;
    unsigned int vao_tutte_as_tex;
    unsigned int vao_lscm;
    unsigned int vao_lscm_as_tex;

    vec3 draw_color_left;
    vec3 draw_color_right;
    vec3 draw_color;
    float draw_radius;
    vec3 clear_color;
    DrawImage image_tutte;
    DrawImage image_lscm;
    unsigned int checker_texture;

    ViewName current_view;
};
