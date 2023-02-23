#define NET_IMPLEMENTATION
#include "debugger.hpp"

int main() {
    auto& debugger = debugger::VisualDebugger::Instance();

    debugger->SendPoint({400, 400, 0}, {0, 255, 0});
    debugger->SendLine({50, 100, 0}, {400, 300, 0}, {255, 0, 0});
    debugger->SendPlane({{50, 100, 0}, {400, 300, 0}, {200, 100}}, {255, 0, 0});

    return 0;
}