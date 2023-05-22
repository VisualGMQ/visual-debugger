#include "shader.hpp"

GLenum type2gl(ShaderModule::Type type) {
    switch (type) {
        case ShaderModule::Type::Vertex: return GL_VERTEX_SHADER;
        case ShaderModule::Type::Fragment: return GL_FRAGMENT_SHADER;
    }
}

std::string_view type2str(ShaderModule::Type type) {
    switch (type) {
        case ShaderModule::Type::Vertex: return "Vertex";
        case ShaderModule::Type::Fragment: return "Fragment";
    }
    return "Unkown";
}

ShaderModule::ShaderModule(Type type, const std::string& code): type_(type) {
    id_ = glCreateShader(type2gl(type));
    const char* source = code.c_str();
    GL_CALL(glShaderSource(id_, 1, &source, nullptr));
    GL_CALL(glCompileShader(id_));

    int success;
    char infoLog[1024];
    GL_CALL(glGetShaderiv(id_, GL_COMPILE_STATUS, &success));
    if(!success) {
        GL_CALL(glGetShaderInfoLog(id_, 1024, NULL, infoLog));
        LOGF("[GL] :", type2gl(type), " shader compile failed:\r\n", infoLog);
    }
}

ShaderModule::~ShaderModule() {
    GL_CALL(glDeleteShader(id_));
}

Shader::Shader(const ShaderModule& vertex, const ShaderModule& fragment) {
    id_ = glCreateProgram();

    GL_CALL(glAttachShader(id_, vertex.id_));
    GL_CALL(glAttachShader(id_, fragment.id_));
    GL_CALL(glLinkProgram(id_));

    int success;
    char infoLog[1024];
    GL_CALL(glGetProgramiv(id_, GL_LINK_STATUS, &success));
    if(!success) {
        glGetProgramInfoLog(id_, 1024, NULL, infoLog);
        LOGF("[GL]: shader link failed:\r\n", infoLog);
    }
}

Shader::~Shader() {
    GL_CALL(glDeleteProgram(id_));
}

void Shader::SetMat4(std::string_view name, const glm::mat4& m) {
    auto loc = glGetUniformLocation(id_, name.data());
    if (loc == -1) {
        LOGE("[GL]: don't has uniform ", name);
    } else {
        GL_CALL(glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(m)));
    }
}

void Shader::SetVec3(std::string_view name, const glm::vec3& v) {
    auto loc = glGetUniformLocation(id_, name.data());
    if (loc == -1) {
        LOGE("[GL]: don't has uniform ", name);
    } else {
        GL_CALL(glUniform3f(loc, v.x, v.y, v.z));
    }
}