/***************************************************************************************************
 * @file  tutte.hpp
 * @brief Declaration of tutte
 **************************************************************************************************/

#pragma once

#include <vector>
#include "mesh_io.h"
#include "vec.h"

using Edge = std::pair<unsigned int, unsigned int>;

std::vector<unsigned int> find_seam(const std::vector<unsigned int>& indices);

std::vector<Point> tutte_uv_unwrapping(const MeshIOData& data);
