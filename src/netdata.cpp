#include "netdata.hpp"

NetSender::NetSender(std::unique_ptr<net::Net>& net, uint32_t port) {
    auto result = net::AddrInfoBuilder::CreateTCP("localhost", port).Build();
    if (result.result != 0) {
        LOGE("create tcp on localhost:", port, " failed!");
    }

    socket_ = net->CreateSocket(result.value);
    socket_->Bind();
    socket_->Connect();
}

NetSender::~NetSender() {
    socket_->Close();
}

void NetSender::SendPacket(const Packet& packet) {
    static std::vector<uint8_t> buf;
    buf.clear();
    buf.push_back('D');
    buf.push_back('B');
    buf.push_back('G');
    buf.push_back('S');

    buf.push_back(static_cast<uint8_t>(packet.type));
    buf.push_back(packet.size);
    size_t oldSize = buf.size();
    buf.resize(buf.size() + 
                packet.data.positions.size() * sizeof(double) +
                sizeof(double) * 3 + // color
                packet.data.name.length() * sizeof(uint8_t));
    double* ptr = (double*)(buf.data() + oldSize);
    for (const auto& pos: packet.data.positions) {
        double x = pos.x;
        double y = pos.y;
        double z = pos.z;
        memcpy(ptr, &x, sizeof(double));
        ptr ++;
        memcpy(ptr, &y, sizeof(double));
        ptr ++;
        memcpy(ptr, &z, sizeof(double));
        ptr ++;
    }
    double r = packet.data.color.r;
    double g = packet.data.color.g;
    double b = packet.data.color.b;
    memcpy(ptr, &r, sizeof(double));
    ptr ++;
    memcpy(ptr, &g, sizeof(double));
    ptr ++;
    memcpy(ptr, &b, sizeof(double));
    ptr ++;
    uint8_t* cp = (uint8_t*)ptr;
    for (auto c : packet.data.name) {
        *cp = c;
    }

    buf.push_back('D');
    buf.push_back('E');
    buf.push_back('G');

    socket_->Send((char*)buf.data(), buf.size());
}