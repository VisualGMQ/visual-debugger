#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "netdata.hpp"

TEST_CASE("Packet Serialize and Deserailize") {
    Packet packet;
    packet.type = Mesh::Type::Triangles;
    NetData data;
    data.name = "packet1";
    data.positions.push_back(Vec3(1, 2, 3));
    data.positions.push_back(Vec3(4, 5, 6));
    data.positions.push_back(Vec3(7, 8, 9));
    data.color = Vec3(0.2, 0.5, 0.7);
    packet.data = data;

    auto datas = packet.Serialize();

    Packet depacket = Packet::Deserialize(datas.data(), datas.data() + datas.size()).value();
    REQUIRE(depacket.type == Mesh::Type::Triangles);
    REQUIRE(depacket.data.positions.size() == 3);
    REQUIRE(depacket.data.name == "packet1");
    REQUIRE(depacket.data.color == Vec3(0.2, 0.5, 0.7));
    REQUIRE(depacket.data.positions[0] == Vec3(1, 2, 3));
    REQUIRE(depacket.data.positions[1] == Vec3(4, 5, 6));
    REQUIRE(depacket.data.positions[2] == Vec3(7, 8, 9));
}