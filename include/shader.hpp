#pragma once

#include "pch.hpp"

class Shader;

class ShaderModule final {
public:
    friend class Shader;

    enum class Type {
        Vertex,
        Fragment,
    };

    ShaderModule(Type type, const std::string& code);
    ~ShaderModule();

private:
    GLuint id_ = 0;
    Type type_;
};

class Shader final {
public:
    Shader(const ShaderModule& vertex, const ShaderModule& fragment);
    ~Shader();

    void Use() { GL_CALL(glUseProgram(id_)); }
    void Unuse() { GL_CALL(glUseProgram(0)); }

    void SetMat4(std::string_view name, const glm::mat4& m);
    void SetVec3(std::string_view name, const glm::vec3& v);

private:
    GLuint id_ = 0;
};