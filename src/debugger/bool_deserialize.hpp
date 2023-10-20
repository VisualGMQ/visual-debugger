#pragma once

#include "mesh.hpp"
#include <iterator>
#include <sstream>

Mesh DeserializeMesh(const std::string& filename);