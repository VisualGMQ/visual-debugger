#include "renderer.hpp"

std::unique_ptr<Renderer> Renderer::instance_ = nullptr;

void Renderer::Init() {
    instance_ = std::make_unique<Renderer>();
}

void Renderer::Quit() {
    instance_.reset();
}

Renderer& Renderer::Instance() {
    return *instance_;
}

const char* FragSource = R"(
    #version 410 core

    out vec4 FragColor;

    uniform vec3 color;

    void main() {
        FragColor = vec4(color, 1.0);
    }
)";

const char* VertexSource = R"(
    #version 410 core

    layout (location = 0) in vec3 inPosition;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 project;

    void main() {
        gl_Position = project * view * model * vec4(inPosition, 1.0);
    }
)";


Renderer::Renderer() {
    GL_CALL(glEnable(GL_DEPTH_TEST));
    GL_CALL(glEnable(GL_STENCIL_TEST));
    GL_CALL(glEnable(GL_LINE_SMOOTH));

    camera_ = std::make_unique<Camera>(Camera::CreatePerspective(Frustum{
        0.00001f, 100000.0f, glm::radians(45.0f), WindowWidth / float(WindowHeight),
    }));

    shader_ = std::make_unique<Shader>(
                ShaderModule(ShaderModule::Type::Vertex, VertexSource),
                ShaderModule(ShaderModule::Type::Fragment, FragSource)); 

    arrayBuffer_ = std::make_unique<Buffer>(Buffer::Type::Array);
    indicesBuffer_ = std::make_unique<Buffer>(Buffer::Type::Element);
    arrayBuffer_->Bind();
    indicesBuffer_->Bind();

    // vertex attributes:
    GL_CALL(glGenVertexArrays(1, &vao_));
    GL_CALL(glBindVertexArray(vao_));
    GL_CALL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0));
    GL_CALL(glEnableVertexAttribArray(0));
    shader_->Use();
}

Renderer::~Renderer() {
    GL_CALL(glDeleteVertexArrays(1, &vao_));
}

void Renderer::Start(const glm::vec3& color) {
    GL_CALL(glClearColor(color.r, color.g, color.b, 1.0));
    GL_CALL(glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT));

    shader_->SetMat4("view", camera_->View());
    shader_->SetMat4("project", camera_->Projection());
}

GLenum meshtype2gl(Mesh::Type type) {
    switch (type) {
        case Mesh::Type::Triangles: return GL_TRIANGLES;
        case Mesh::Type::Lines : return GL_LINES;
        case Mesh::Type::LineStrip : return GL_LINE_STRIP;
        case Mesh::Type::LineLoop : return GL_LINE_LOOP;
        case Mesh::Type::Points: return GL_POINTS;
    }
}

void Renderer::Draw(const Mesh& mesh, const glm::mat4& model, const glm::vec3& color) {
    shader_->SetMat4("model", model);
    shader_->SetVec3("color", color);

    arrayBuffer_->SetData((void*)mesh.vertices.data(), sizeof(Vertex) * mesh.vertices.size());

    if (mesh.type == Mesh::Type::Points) {
        GL_CALL(glPointSize(5));
    } else {
        GL_CALL(glPointSize(1));
    }
    if (mesh.indices) {
        indicesBuffer_->SetData((void*)mesh.indices.value().data(), mesh.indices.value().size() * sizeof(uint32_t));
        glDrawElements(meshtype2gl(mesh.type), mesh.indices.value().size(), GL_UNSIGNED_INT, 0);
    } else {
        glDrawArrays(meshtype2gl(mesh.type), 0, mesh.vertices.size());
    }
}