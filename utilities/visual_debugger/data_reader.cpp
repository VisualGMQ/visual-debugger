#include "draw_commands.hpp"
#include <functional>
#include <vector>

using PointDealFun = std::function<void(const debugger::Vec3&)>;
using LineDealFun = std::function<void(const debugger::Vec3&, const debugger::Vec3&)>;
using PlaneDealFun = std::function<void(const std::vector<debugger::Vec3>&)>;

void ReadFromFile(const std::string filename, PointDealFun pt, LineDealFun line, PlaneDealFun plane) {
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
            line(v1, v2);
        } else if (type == "plane") {
            int count;
            file >> count;
            std::vector<debugger::Vec3> vertices;
            for (int i = 0; i < count; i++) {
                debugger::Vec3 v = debugger::Vec3::FromFile(file);
                vertices.push_back(v);
            }
            plane(vertices);
        }
    }
}

int main(int argc, char** argv) {
    ReadFromFile("./debugger-log.txt", [](const debugger::Vec3& v){
        std::cout << v;
    },
    [](const debugger::Vec3& v1, const debugger::Vec3& v2) {

    }, [](const std::vector<debugger::Vec3>&) {});
}