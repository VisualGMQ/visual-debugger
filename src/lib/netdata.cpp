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
    auto buf = packet.Serialize();
    if (socket_->Send("BEG", 3) <= 0) {
        LOGI("connect lost");
    }
    if (socket_->Send((char*)buf.data(), buf.size()) <= 0) {
        LOGI("connect lost");
    }
    if (socket_->Send("END", 3) <= 0) {
        LOGI("connect lost");
    }
}

NetRecv::NetRecv(std::unique_ptr<net::Net>& net, uint32_t port) {
    auto result = net::AddrInfoBuilder::CreateTCP("localhost", port).Build();
    if (result.result != 0) {
        LOGE("create tcp on localhost:", port, " failed!");
    }

    socket_ = net->CreateSocket(result.value);
    socket_->Bind();
    socket_->Listen(1);
    LOGI("listening on ", port, "...");

    auto client = socket_->Accept();
    client_ = std::move(client.value);
    LOGI("connected client");
    if (!client_->Valid()) {
        LOGI("client not valid");
    }
}

NetRecv::~NetRecv() {
    client_->Close();
    socket_->Close();
}

std::vector<Packet> NetRecv::RecvPacket() {
    if (!client_) {
        return {};
    }

    uint8_t buf[1024] = {0};
    int len = client_->Recv((char*)buf, sizeof(buf));
    LOGI("len = ", len);

    if (len <= 0) {
        LOGI("client closed");
        client_->Close();
        client_ = nullptr;
        return {};
    }

    std::vector<Packet> packets;
    auto [result, nextPtr] = splitOnePacket(buf, buf + len);
    while (nextPtr < buf + len) {
        if (result) {
            auto packet = analyzePacket(cache_.data(), cache_.data() + cache_.size());
            if (!packet) {
                LOGE("analyze packet failed!");
            } else {
                packets.push_back(packet.value());
            }
        }
        auto sp = splitOnePacket(nextPtr, buf + len);
        result = sp.first;
        nextPtr = sp.second;
    }
	if (result) {
		auto packet = analyzePacket(cache_.data(), cache_.data() + cache_.size());
		if (!packet) {
			LOGE("analyze packet failed!");
		} else {
			packets.push_back(packet.value());
		}
	}

    return packets;
}

std::pair<bool, const uint8_t*> NetRecv::splitOnePacket(const uint8_t* beg, const uint8_t* end) {
    const uint8_t* begPtr = FindSubStr(beg, end - beg, (uint8_t*)"BEG", 3);
    const uint8_t* endPtr = FindSubStr(beg, end - beg, (uint8_t*)"END", 3);

    if (begPtr == nullptr && endPtr == nullptr) {
        cache_.insert(cache_.end(), beg, end);
        return {false, end};
    } else if (begPtr == nullptr && endPtr != nullptr) {
        cache_.insert(cache_.end(), beg, endPtr);
        return {true, endPtr + 3};
    } else if (begPtr != nullptr && endPtr == nullptr) {
        if (!cache_.empty()) {
            LOGE("[NET]: last packet in cache not finish, but recv a BEG!");
            cache_.clear();
        }
        cache_.insert(cache_.end(), begPtr + 3, end);
        return {false, end};
    } else {
        if (begPtr < endPtr) {
            if (!cache_.empty()) {
				LOGE("[NET]: last packet in cache not finish, but recv a BEG!");
				cache_.clear();
            }

            cache_.insert(cache_.end(), begPtr + 3, endPtr);
            return {true, endPtr + 3};
        } else {
            cache_.insert(cache_.end(), beg, begPtr);
            return {true, begPtr};
        }
    }
}


std::optional<Packet> NetRecv::analyzePacket(const uint8_t* beg, const uint8_t* end) {
    return Packet::Deserialize(beg, end);
}

std::vector<uint8_t> Packet::Serialize() const {
    static std::vector<uint8_t> buf;
    buf.clear();

    buf.push_back(static_cast<uint8_t>(type));
    buf.push_back(data.positions.size());
    size_t oldSize = buf.size();
    buf.resize(oldSize +
               data.positions.size() * sizeof(double) * 3 +
               sizeof(double) * 3 + // color
               data.name.length() * sizeof(uint8_t));
    double* ptr = (double*)(buf.data() + oldSize);
    for (const auto& pos: data.positions) {
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
    double r = data.color.r;
    double g = data.color.g;
    double b = data.color.b;
    memcpy(ptr, &r, sizeof(double));
    ptr ++;
    memcpy(ptr, &g, sizeof(double));
    ptr ++;
    memcpy(ptr, &b, sizeof(double));
    ptr ++;
    uint8_t* cp = (uint8_t*)ptr;
    for (auto c : data.name) {
        *cp = c;
        cp ++;
    }

    return buf;
}

std::optional<Packet> Packet::Deserialize(const uint8_t* beg, const uint8_t* end) {
    const uint8_t* ptr = beg;
    Mesh::Type type = static_cast<Mesh::Type>(*ptr);
    ptr ++;
    uint8_t count = *ptr;
    ptr ++;

    std::vector<Vec3> positions;
    // serialize positions
    for (int i = 0; i < count; i++) {
        double x, y, z;
        memcpy(&x, ptr, sizeof(double));
        ptr += sizeof(double);
        memcpy(&y, ptr, sizeof(double));
        ptr += sizeof(double);
        memcpy(&z, ptr, sizeof(double));
        ptr += sizeof(double);
        positions.push_back(Vec3(x, y, z));
    }

    // serialize color
    double x, y, z;
    memcpy(&x, ptr, sizeof(double));
    ptr += sizeof(double);
    memcpy(&y, ptr, sizeof(double));
    ptr += sizeof(double);
    memcpy(&z, ptr, sizeof(double));
    ptr += sizeof(double);

    // serialize name
    std::string name;
    while (ptr < end) {
        name.push_back(*ptr);
        ptr ++;
    }

    // compose packet
    NetData data;
    data.positions = positions;
    data.color = Vec3(x, y, z);
    data.name = name;
    return Packet {type, data};
}