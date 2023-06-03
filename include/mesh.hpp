#pragma once

#include "pch.hpp"
#include "vertex.hpp"

struct Mesh final {
    enum Type: uint8_t {
		Triangles = 0,
		Lines = 1,
		LineStrip = 2,
		LineLoop = 3,
		Points = 4,
    };

    std::vector<Vertex> vertices;
    std::optional<std::vector<uint32_t>> indices;
    Type type;

    static Mesh Create(Type type, const std::vector<Vertex>& vertices) {
        return Mesh{vertices, std::nullopt, type};
    }

    static Mesh CreateWithIndices(Type type, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
        return Mesh{vertices, indices, type};
    }
};
