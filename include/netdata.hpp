#pragma once

#include "pch.hpp"
#include "mesh.hpp"
#include "net.hpp"

using Vec3 = glm::vec3;

struct NetData final {
    std::vector<Vec3> positions;
    glm::vec3 color;
    std::string name;
};

struct Packet final {
    Mesh::Type type;
    NetData data;

    std::vector<uint8_t> Serialize() const;
    static std::optional<Packet> Deserialize(const uint8_t* beg, const uint8_t* end);
};

class NetSender final {
public:
    NetSender(std::unique_ptr<net::Net>& net, uint32_t port);
    ~NetSender();
    void SendPacket(const Packet&);

private:
    net::Socket* socket_;
};

class NetRecv final {
public:
    NetRecv(std::unique_ptr<net::Net>& net, uint32_t port);
    ~NetRecv();
    std::vector<Packet> RecvPacket();
    void TryAccept();

private:
    net::Socket* socket_;
    std::vector<uint8_t> cache_;
    std::unique_ptr<net::Socket> client_;

    std::optional<Packet> analyzePacket(const uint8_t* beg, const uint8_t* end);
    std::pair<bool, const uint8_t*> splitOnePacket(const uint8_t* beg, const uint8_t* end);
};
