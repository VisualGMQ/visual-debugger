#include "pch.hpp"
#include "net.hpp"
#include "netdata.hpp"
#include <thread>
#include <chrono>

int main() {
    auto net = net::Init();

    NetSender sender(net, 8999);

    Packet packet;
    packet.type = Mesh::Type::LineLoop;
    NetData data;
    data.name = "packet1";
    data.positions.push_back(Vec3(-0.5, 0, -1));
    data.positions.push_back(Vec3(0.5, 0, -1));
    data.positions.push_back(Vec3(0, 0, -0.5));
    data.color = Vec3(0.2, 0.5, 0.7);
    packet.data = data;

    sender.SendPacket(packet);

    for (int i = 0; i < 1000; i++) {
        Packet packet2;
        packet2.type = Mesh::Type::Points;
        NetData data;
        data.name = "points" + std::to_string(i);
        data.positions.push_back(Vec3(-0.5, 0, -1));
        data.color = Vec3(0.2, 0.5, 0.7);
        packet2.data = data;

        sender.SendPacket(packet2);
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));

    return 0;
}