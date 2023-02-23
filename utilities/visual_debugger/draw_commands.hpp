#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <iostream>
#include <fstream>

namespace debugger {

/*
Serialize Protocol:

Type:   Point
Format: Type x y z
Bytes:  1    4 4 4
TotleBytes: 13

Type:   Line
Format: Type x1 y1 z2 x2 y2 z2
Bytes:  1    4  4  4  4  4  4
TotleBytes: 25

Type:  Plane
Format Type num [x y z]*
Bytes: 1    8    4 4 4
TotleBytes: 9 + n * (12)
*/

using Uint8Ptr = uint8_t*;

void PutBuf(Uint8Ptr& buf, void* data, size_t size) {
    memcpy(buf, data, size);
    buf += size;
}

void GetBuf(Uint8Ptr& buf, void* data, size_t size) {
    memcpy(data, buf, size);
    buf += size;
}

enum class CmdType : uint8_t {
    Unknown,
    Line,
    Plane,
    Point,
};

struct Vec3 final {
    float x, y, z;

    static size_t SerialSize() {
        return 4 + 4 + 4;
    }

    static Vec3 Deserialize(Uint8Ptr& buf) {
        Vec3 result;

        GetBuf(buf, &result.x, 4);
        GetBuf(buf, &result.y, 4);
        GetBuf(buf, &result.z, 4);

        return result;
    }

    void Serialize(Uint8Ptr& buf) {
        PutBuf(buf, &x, 4);
        PutBuf(buf, &y, 4);
        PutBuf(buf, &z, 4);
    }

    void Output2File(std::ofstream& file) {
        file << x << " " << y << " " << z << " ";
    }

    static Vec3 FromFile(std::ifstream& file) {
        Vec3 v;
        file >> v.x >> v.y >> v.z;
        return v;
    }
};

inline std::ostream& operator<<(std::ostream& o, const Vec3& v) {
    o << "Vec3[" << v.x << ", " << v.y << ", " << v.z << "]";
    return o;
}

struct Color final {
    uint8_t r, g, b;

    Color(): r(0), g(0), b(0) {}
    Color(uint8_t r, uint8_t g, uint8_t b): r(r), g(g), b(b) {}

    static size_t SerialSize() {
        return 1 + 1 + 1;
    }

    static Color Deserialize(Uint8Ptr& buf) {
        Color result;

        GetBuf(buf, &result.r, 1);
        GetBuf(buf, &result.g, 1);
        GetBuf(buf, &result.b, 1);

        return result;
    }

    void Serialize(Uint8Ptr& buf) {
        PutBuf(buf, &r, 1);
        PutBuf(buf, &g, 1);
        PutBuf(buf, &b, 1);
    }

};

inline std::ostream& operator<<(std::ostream& o, const Color& v) {
    o << "Color[" << static_cast<int>(v.r) << ", " << static_cast<int>(v.g) << ", " << static_cast<int>(v.b) << "]";
    return o;
}

struct LineCmd final {
    Vec3 a, b;
    Color color; 

    static size_t SerialSize() {
        return sizeof(CmdType) + Vec3::SerialSize() + Vec3::SerialSize() + Color::SerialSize();
    }

    static LineCmd Deserialize(Uint8Ptr& buf) {
        LineCmd result;

        result.a = Vec3::Deserialize(buf);
        result.b = Vec3::Deserialize(buf);
        result.color = Color::Deserialize(buf);

        return result;
    }

    void Serialize(Uint8Ptr& buf) {
        CmdType type = CmdType::Line;

        PutBuf(buf, &type, sizeof(CmdType));
        a.Serialize(buf);
        b.Serialize(buf);
        color.Serialize(buf);
    }
};

struct PlaneCmd final {
    Color color;
    std::vector<Vec3> vertices;

    size_t SerialSize() const {
        return sizeof(CmdType) + color.SerialSize() + sizeof(uint64_t) + vertices.size() * Vec3::SerialSize();
    }

    static PlaneCmd Deserialize(Uint8Ptr& buf) {
        PlaneCmd result;
        uint64_t count;
        GetBuf(buf, &count, sizeof(count));
        result.vertices.resize(count);

        for (int i = 0; i < count; i++) {
            result.vertices[i] = Vec3::Deserialize(buf);
        }

        result.color = Color::Deserialize(buf);

        return result;
    }

    void Serialize(Uint8Ptr& buf) {
        CmdType type = CmdType::Plane;

        PutBuf(buf, &type, sizeof(CmdType));
        uint64_t count = vertices.size();
        PutBuf(buf, &count, sizeof(count));
        for (auto& vertex : vertices) {
            vertex.Serialize(buf);
        }
        color.Serialize(buf);
    }
};

struct PointCmd final {
    Vec3 point;
    Color color;

    static PointCmd Deserialize(Uint8Ptr& buf) {
        PointCmd result;

        result.point = Vec3::Deserialize(buf);
        result.color = Color::Deserialize(buf);

        return result;
    }

    size_t SerialSize() const {
        return sizeof(CmdType) + point.SerialSize() + color.SerialSize();
    }

    void Serialize(Uint8Ptr& buf) {
        CmdType type = CmdType::Point;

        PutBuf(buf, &type, sizeof(CmdType));
        point.Serialize(buf);
        color.Serialize(buf);
    }
};

}