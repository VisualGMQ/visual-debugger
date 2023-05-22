#include "pch.hpp"
#include "renderer.hpp"
#include <thread>
#include <mutex>
#include "netdata.hpp"
#include "vertex.hpp"

float gRotateY = 0;
glm::vec3 gOrigin;
float gScale = 1.0;

void ErrorCallback(int error, const char* description) {
    LOGE("[GLFW]: ", error, " - ", description);
}

void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
    static double oldX, oldY;
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        gRotateY += (xpos - oldX) * 0.001;
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        gOrigin.x -= (xpos - oldX) * 0.01;
        gOrigin.y += (ypos - oldY) * 0.01;
    }

    oldX = xpos;
    oldY = ypos;
}

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    LOGI("here");
    gScale += 0.01 * (yoffset > 0 ? 1.0 : -1.0);
    if (gScale < 0.1) {
        gScale = 0.1;
    }
    LOGI("gScale = ", gScale);
}

struct RenderData {
    Mesh mesh;
    glm::vec3 color;
};

std::unordered_map<std::string, RenderData> gDatas;

int main() {
    if (!glfwInit()) {
        LOGF("[APP]: glfw init failed");
        return 1;
    }

    glfwSetErrorCallback(ErrorCallback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    GLFWwindow* window = glfwCreateWindow(1024, 720, "VisualDebugger", NULL, NULL);
    if (!window) {
        LOGE("[GLFW]: create window failed");
    }

    glfwMakeContextCurrent(window);
    if (gladLoadGL() == 0) {
        LOGF("[GLAD]: glad init failed!");
        return 2;
    }

    glfwSetCursorPosCallback(window, CursorPositionCallback);
    glfwSetScrollCallback(window, ScrollCallback);

    Renderer::Init();
    auto& renderer = Renderer::Instance();
    auto& camera = renderer.GetCamera();
    camera.MoveTo(glm::vec3(0, 5, 5));
    camera.Lookat(glm::vec3(0, 0, 0));

    std::thread th([&](){
        std::mutex m;
        auto net = net::Init();
        NetRecv recv(net, 8999);
        while (true) {
            auto packets = recv.RecvPacket();
            for (const auto& packet : packets) {
                LOGI("recv ", packet.data.name);
                std::vector<Vertex> vertices;
                for (const auto& pos : packet.data.positions) {
                    vertices.push_back(Vertex{pos});
                }
                Mesh mesh = Mesh::Create(packet.type, vertices);
                std::lock_guard guard(m);
                gDatas[packet.data.name] = RenderData{mesh, packet.data.color};
            }
        }
    });

    std::mutex m;
    while (!glfwWindowShouldClose(window)) {
        renderer.Start(glm::vec3(0.2, 0.2, 0.2));

        std::lock_guard guard(m);
        for (const auto& data : gDatas) {
            renderer.Draw(data.second.mesh, 
                            glm::rotate(
                                glm::scale(
                                    glm::translate(glm::mat4(1.0), -gOrigin),
                                    glm::vec3(gScale, gScale, gScale)),
                                gRotateY, glm::vec3(0, 1, 0)),
                        data.second.color);
        }

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    Renderer::Quit();
    glfwDestroyWindow(window);
    glfwTerminate();

    th.join();
    return 0;
}