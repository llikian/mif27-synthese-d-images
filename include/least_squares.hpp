/***************************************************************************************************
 * @file  least_squares.hpp
 * @brief Declaration of least_squares
 **************************************************************************************************/

#pragma once

#include <vector>
#include "mesh_io.h"
#include "vec.h"

std::vector<Point> least_squares_uv_unwrapping(const MeshIOData& data);
