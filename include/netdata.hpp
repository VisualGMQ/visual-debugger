#pragma once

#include "pch.hpp"
#include "mesh.hpp"

#define NET_IMPLEMENTATION
#include "net.hpp"

using Vec3 = glm::vec3;

struct NetData final {
    std::vector<Vec3> positions;
    glm::vec3 color;
    std::string name;
};

struct Packet final {
    Mesh::Type type;
    unsigned int size;
    NetData data;
};

class NetSender final {
public:
    NetSender(std::unique_ptr<net::Net>& net, uint32_t port);
    ~NetSender();
    void SendPacket(const Packet&);

private:
    net::Socket* socket_;
};

void SendPacket(net::Socket& socket, const Packet&);
void RecvPacket(net::Socket& socket, const Packet&);