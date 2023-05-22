#pragma once

#include "pch.hpp"

struct Frustum final {
    float near;
    float far;
    float fov;
    float aspect;
};

class Camera final {
public:
    static Camera CreatePerspective(const Frustum&);

    void MoveTo(const glm::vec3& position) {
        position_ = position;
        view_ = glm::lookAt(position_, target_, glm::vec3(0, 1, 0));
    }
    void Move(const glm::vec3& offset) {
        position_ += offset;
        view_ = glm::lookAt(position_, target_, glm::vec3(0, 1, 0));
    }

    void Lookat(const glm::vec3& target) {
        target_ = target;
        view_ = glm::lookAt(position_, target_, glm::vec3(0, 1, 0));
    }

    const glm::mat4& Projection() const { return proj_; }
    const glm::mat4& View() const { return view_; }

private:
    glm::mat4 proj_ = glm::mat4(1.0);
    glm::mat4 view_ = glm::mat4(1.0);

    glm::vec3 position_ = glm::vec3(0, 0, 0);
    glm::vec3 target_ = { 0.0f, 0.0f, -1.0f };

    explicit Camera(const glm::mat4& proj): proj_(proj) {}
};