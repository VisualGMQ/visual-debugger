#include "bool_deserialize.hpp"
#include "netdata.hpp"
#include <string>

enum class ParseMode {
    None,
    VertexInfo,
    FaceInfo,
};

std::vector<std::string_view> Split(std::string_view str, char delim) {
    std::vector<std::string_view> result;

    size_t idx = 0;
    while (idx != str.npos) {
        size_t newIdx = str.find(delim, idx);
        if (newIdx != str.npos) {
            result.push_back(str.substr(idx, newIdx- idx));
            idx = newIdx + 1;
        } else {
            result.push_back(str.substr(idx, str.length() - idx));
            idx = str.npos;
        }
    }

    return result;
}

Mesh DeserializeMesh(const std::string& filename) {
    std::ifstream file(filename);
    if (file.fail()) {
        return {};
    }

    ParseMode mode = ParseMode::None;
    Mesh mesh;
    mesh.type = Mesh::Type::Lines;

    char line[4096];
    while (file.getline(line, sizeof(line))) {
        std::string_view l = line;
        if (auto idx = l.find("vertexInfo:"); idx != l.npos) {
            mode = ParseMode::VertexInfo;
            l = l.substr(idx + 11);
        }
        if (auto idx = l.find("faceInfo:"); idx != l.npos) {
            mode = ParseMode::FaceInfo;
            l = l.substr(idx + 9, l.length());
        }

        if (mode == ParseMode::VertexInfo) {
            l = l.substr(0, l.length() - 1);
            auto vecStr = Split(l, ',');
            Vec3 v;
            v.x = std::stof(std::string(vecStr[0]));
            v.y = std::stof(std::string(vecStr[1]));
            v.z = std::stof(std::string(vecStr[2]));
            mesh.vertices.push_back({v});
        } else if (mode == ParseMode::FaceInfo) {
            auto indicesStr = Split(l, ',');
            std::vector<int> indices;
            std::vector<uint32_t> realIndices;

            for (auto s : indicesStr) {
                indices.push_back(std::stoi(std::string(s)));
            }

            size_t idx = 0;
            while (idx < indices.size()) {
                int edgeCount = indices[idx++];

                for (int i = 0; i < edgeCount - 1; i++) {
                    realIndices.push_back(indices[idx + i]);
                    realIndices.push_back(indices[idx + i + 1]);
                }

                realIndices.push_back(indices[idx + edgeCount - 1]);
                realIndices.push_back(indices[idx]);
                idx += edgeCount;
            }

            mesh.indices = realIndices;
        }
    }

    return mesh;
}