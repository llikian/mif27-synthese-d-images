/***************************************************************************************************
 * @file  lscm.hpp
 * @brief Declaration of lscm
 **************************************************************************************************/

#pragma once

#include <vector>
#include "mesh_io.h"
#include "vec.h"

std::vector<Point> lscm_uv_unwrapping(const MeshIOData& data);
