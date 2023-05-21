#pragma once

#include "pch.hpp"

class Buffer final {
public:
    enum Type {
        Array,
        Element,
    };

    Buffer(Type type);
    ~Buffer();

    void SetData(void* datas, size_t size);

    void Bind();
    void Unbind();

private:
    GLuint id_;
    Type type_;
};