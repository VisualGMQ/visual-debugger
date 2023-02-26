#pragma once

#include "net.hpp"
#include "draw_commands.hpp"
#include <thread>
#include <chrono>

namespace debugger {

constexpr uint16_t PORT = 12757;

class VisualDebugger {
public:
    static std::unique_ptr<VisualDebugger>& Instance() {
        static std::unique_ptr<VisualDebugger> instance;

        if (!instance) {
            instance = std::make_unique<VisualDebugger>();
        }

        return instance;
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

    ~VisualDebugger() {
        int code = 0;
        if (socket_->Close(SD_SEND) != 0) {
            std::cerr << "close socket failed: " << net::GetLastError() << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
        net_.reset();
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

    void SendLine(const Vec3& a, const Vec3& b, float bulge, const Color& color) {
        if (!socket_ || !socket_->Valid()) return;

        auto cmd = LineCmd();
        cmd.color = color;
        cmd.a = a;
        cmd.b = b;
        cmd.bulge = bulge;

        uint8_t buf[1024] = {0};

        Uint8Ptr ptr = buf;
        cmd.Serialize(ptr);
        int iResult = socket_->Send((char*)buf, cmd.SerialSize());
    }

    void SendPlane(const std::vector<Vec3>& vertices, const std::vector<float>& bulges, const Color& color) {
        if (!socket_ || !socket_->Valid()) return;

        auto cmd = PlaneCmd();
        cmd.color = color;
        cmd.vertices = vertices;
        cmd.bulges = bulges;

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