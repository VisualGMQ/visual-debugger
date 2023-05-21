#pragma once

#include "buffer.hpp"

GLenum type2gl(Buffer::Type type) {
    switch (type) {
        case Buffer::Type::Array: return GL_ARRAY_BUFFER;
        case Buffer::Type::Element: return GL_ELEMENT_ARRAY_BUFFER;
    }
}

std::string_view type2str(Buffer::Type type) {
    switch (type) {
        case Buffer::Type::Array: return "Array";
        case Buffer::Type::Element: return "Element";
    }
    return "Unknown";
}

Buffer::Buffer(Type type): type_(type) {
    GL_CALL(glGenBuffers(1, &id_));
}

void Buffer::Bind() {
    glBindBuffer(type2gl(type_), id_);
}

void Buffer::Unbind() {
    glBindBuffer(type2gl(type_), 0);
}

void Buffer::SetData(void* datas, size_t size) {
    Bind();
    GL_CALL(glBufferData(type2gl(type_), size, datas, GL_STATIC_DRAW));
}

Buffer::~Buffer() {
    GL_CALL(glDeleteBuffers(1, &id_));
}