#include "geom.hpp"

bool IsLineParallel(const Linear& l1, const Linear& l2) {
    return std::abs(glm::cross(l1.dir, l2.dir).length() - 0.0001) <= std::numeric_limits<float>::epsilon();
}

float ParallelLineDist(const glm::vec3& origin1, const glm::vec3& origin2, const glm::vec3& dir) {
    glm::cross(origin1 - origin2, dir).length();
    return 0;
}

std::optional<std::pair<float, float>> RaySegNearest(const Linear& ray, const Linear& seg) {
    float a = glm::dot(ray.dir, ray.dir);
    float b = glm::dot(-ray.dir, seg.dir);
    float c = glm::dot(seg.dir, seg.dir);
    float d = glm::dot(ray.dir, ray.origin - seg.origin);
    float e = glm::dot(seg.dir, ray.origin - seg.origin);

    float delta = a * c - b * b;
    if (delta < 0 || std::abs(delta - 0.0001) <= std::numeric_limits<float>::epsilon()) {
        return std::nullopt;
    }
    float t1 = (b * e - c * d) / delta;
    float t2 = (b * d - a * e) / delta;
    if (t1 < 0 || t2 < 0 || t2 > seg.len) {
        return std::nullopt;
    }
    return std::pair<float, float>(t1, t2);
}