#pragma once

#include "pch.hpp"

struct Linear {
    glm::vec3 origin;
    glm::vec3 dir;
    float len;

    glm::vec3 At(float t) const {
        return origin + t * dir;
    }

    static Linear CreateFromLine(const glm::vec3& origin, const glm::vec3& dir) {
        return Linear { origin, glm::normalize(dir), 1 };
    }

    static Linear CreateFromSeg(const glm::vec3& p1, const glm::vec3& p2) {
        return Linear { p1, glm::normalize(p2 - p1), (float)(p2 - p1).length() };
    }
};

bool IsLineParallel(const Linear& l1, const Linear& l2);
float ParallelLineDist(const glm::vec3& origin1, const glm::vec3& origin2, const glm::vec3& dir);
std::optional<std::pair<float, float>> RaySegNearest(const Linear& ray, const Linear& seg);