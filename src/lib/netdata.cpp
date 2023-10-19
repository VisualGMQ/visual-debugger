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
    if (auto result = socket_->Send("BEG", 3); result.result != 0) {
        LOGI("connect lost: ", net::Error2Str(result.result));
        return;
    }
    if (auto result = socket_->Send((char*)buf.data(), buf.size()); result.result != 0) {
        LOGI("connect lost: ", net::Error2Str(result.result));
        return;
    }
    if (auto result = socket_->Send("END", 3); result.result != 0) {
        LOGI("connect lost: ", net::Error2Str(result.result));
        return;
    }
}

NetRecv::~NetRecv() {
    if (client_) {
		client_->Close();
    }
}

std::vector<Packet> NetRecv::RecvPacket() {
    if (!client_) {
        return {};
    }

    uint8_t buf[4096] = {0};
    auto recvResult = client_->Recv((char*)buf, sizeof(buf));

    if (recvResult.value<= 0) {
        LOGI("client closed: ", net::Error2Str(recvResult.result));
        client_->Close();
        client_ = nullptr;
        return {};
    }

    int len = recvResult.value;

    std::vector<Packet> packets;
    auto [result, nextPtr] = splitOnePacket(buf, buf + len);
    while (nextPtr < buf + len) {
        if (result) {
            auto packet = analyzePacket(cache_.data(), cache_.data() + cache_.size());
            cache_.clear();
            if (!packet) {
                LOGE("analyze packet failed!");
            } else {
                if (packet.value().data.name.empty()) {
                    LOGT("here");
                }
                packets.push_back(packet.value());
            }
        }
        auto sp = splitOnePacket(nextPtr, buf + len);
        result = sp.first;
        nextPtr = sp.second;
    }
	if (result) {
		auto packet = analyzePacket(cache_.data(), cache_.data() + cache_.size());
        cache_.clear();
		if (!packet) {
			LOGE("analyze packet failed!");
		} else {
                if (packet.value().data.name.empty()) {
                    LOGT("here");
                }
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
            if (begPtr - endPtr != 3) {
                LOGE("invalid data between END and BEG");
            }
            cache_.insert(cache_.end(), beg, endPtr);
            return {true, begPtr};
        }
    }
}


std::optional<Packet> NetRecv::analyzePacket(const uint8_t* beg, const uint8_t* end) {
    if (end - beg <= 3) {
        LOGE("packet data not enought!");
        return std::nullopt;
    }
    if (beg[0] == 'B' && beg[1] == 'E' && beg[2] == 'G') {
        return Packet::Deserialize(beg + 3, end);
    }
    auto packet = Packet::Deserialize(beg, end);
    if (packet) {
        for (auto& vertex : packet.value().data.positions) {
            std::swap(vertex.y, vertex.z);
        }
    }

    return packet;
}

std::vector<uint8_t> Packet::Serialize() const {
    std::vector<uint8_t> buf;

    buf.push_back(static_cast<uint8_t>(type));
    size_t oldSize = buf.size();
    buf.resize(oldSize +
               4 +
               data.positions.size() * sizeof(double) * 3 +
               sizeof(double) * 3 + // color
               data.name.length() * sizeof(uint8_t));
    double* ptr = (double*)(buf.data() + oldSize);
    uint32_t count = data.positions.size();
    uint32_t* uptr = (uint32_t*)ptr;
    memcpy(uptr, &count, sizeof(count));
    ptr = (double*)(uptr + 1);
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
    uint32_t count = 0;
    memcpy(&count, ptr, sizeof(count));
    ptr += 4;

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