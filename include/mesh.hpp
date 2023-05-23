#pragma once

#include "pch.hpp"
#include "vertex.hpp"

struct Mesh final {
    enum Type: uint8_t {
        Triangles = 0,
        Lines = 1,
        LineLoop = 2,
        Points = 3,
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
