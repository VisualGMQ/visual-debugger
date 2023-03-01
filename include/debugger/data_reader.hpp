#pragma once
#include <functional>
#include <vector>
#include <fstream>
#include "draw_commands.hpp"

using PointDealFun = std::function<void(const debugger::Vec3&)>;
using LineDealFun = std::function<void(const debugger::Vec3&, const debugger::Vec3&, float)>;
using PlaneDealFun = std::function<void(const std::vector<debugger::Vec3>&, const std::vector<float>&)>;

inline void ReadFromFile(const std::string filename, PointDealFun pt, LineDealFun line, PlaneDealFun plane) {
    std::ifstream file(filename);
    while (!file.eof()) {
        std::string type;
        file >> type;
        if (type == "point") {
            debugger::Vec3 v = debugger::Vec3::FromFile(file);
            pt(v);
        } else if (type == "line") {
            debugger::Vec3 v1 = debugger::Vec3::FromFile(file),
                           v2 = debugger::Vec3::FromFile(file);
            float bulge;
            file >> bulge;
            line(v1, v2, bulge);
        } else if (type == "plane") {
            int count;
            file >> count;
            std::vector<debugger::Vec3> vertices;
            for (int i = 0; i < count; i++) {
                debugger::Vec3 v = debugger::Vec3::FromFile(file);
                vertices.push_back(v);
            }
            std::vector<float> bulges;
            for (int i = 0; i < count; i++) {
                float bulge;
                file >> bulge;
                bulges.push_back(bulge);
            }
            plane(vertices, bulges);
        }
    }
}
