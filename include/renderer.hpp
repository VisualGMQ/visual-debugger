#pragma once

#include "pch.hpp"
#include "camera.hpp"
#include "buffer.hpp"
#include "shader.hpp"
#include "mesh.hpp"

class Renderer final {
public:
    static void Init();
    static void Quit();
    static Renderer& Instance();

    Renderer();
    ~Renderer();

    Camera& GetCamera() { return *camera_; }

    void Start(const glm::vec3& color);

    void SetLineWidth(int width) { GL_CALL(glLineWidth(width)); }

    void Draw(const Mesh& mesh, const glm::mat4& model, const glm::vec3& color);

private:
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<Shader> shader_;
    std::unique_ptr<Buffer> arrayBuffer_;
    std::unique_ptr<Buffer> indicesBuffer_;
    GLuint vao_;

    static std::unique_ptr<Renderer> instance_;
};