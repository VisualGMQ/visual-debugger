#pragma once

#include "net.hpp"
#include "draw_commands.hpp"

namespace debugger {

constexpr uint16_t PORT = 12757;

class VisualDebugger {
public:
    static VisualDebugger& Instance() {
        static std::unique_ptr<VisualDebugger> instance;

        if (!instance) {
            instance = std::make_unique<VisualDebugger>();
        }

        return *instance;
    }

    VisualDebugger() {
        net_ = net::Init();

        auto result = net::AddrInfoBuilder::CreateTCP("localhost", PORT).Build();
        if (result.result != 0) {
            std::cerr << "create TCP addrinfo failed: " << net::GetLastError() << std::endl;
        }

        socket_ = net_->CreateSocket(result.value);
        socket_->Bind();

        std::cout << "connecting..." << std::endl;
        socket_->Connect();
    }

    void SendPoint(const Vec3& pt, const Color& color) {
        if (!socket_ || !socket_->Valid()) return;

        auto pointcmd = PointCmd();
        pointcmd.color = color;
        pointcmd.point = pt;

        uint8_t buf[1024] = {0};

        Uint8Ptr ptr = buf;
        pointcmd.Serialize(ptr);
        int iResult = socket_->Send((char*)buf, pointcmd.SerialSize());
    }

    void SendLine(const Vec3& a, const Vec3& b, const Color& color) {
        if (!socket_ || !socket_->Valid()) return;

        auto cmd = LineCmd();
        cmd.color = color;
        cmd.a = a;
        cmd.b = b;

        uint8_t buf[1024] = {0};

        Uint8Ptr ptr = buf;
        cmd.Serialize(ptr);
        int iResult = socket_->Send((char*)buf, cmd.SerialSize());
    }

    void SendPlane(const std::vector<Vec3>& vertices, const Color& color) {
        if (!socket_ || !socket_->Valid()) return;

        auto cmd = PlaneCmd();
        cmd.color = color;
        cmd.vertices = vertices;

        uint8_t buf[4096] = {0};

        Uint8Ptr ptr = buf;
        cmd.Serialize(ptr);
        int iResult = socket_->Send((char*)buf, cmd.SerialSize());
    }

private:
    std::unique_ptr<net::Net> net_;
    net::Socket* socket_;
};

}