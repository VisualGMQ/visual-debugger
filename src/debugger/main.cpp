#include "pch.hpp"
#include "renderer.hpp"
#include <thread>
#include <mutex>
#include <iostream>
#include "netdata.hpp"
#include "vertex.hpp"
#include "geom.hpp"
#include "camera.hpp"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"


float gRotateY = 0;
glm::vec3 gOrigin;
float gScale = 1.0;
bool gShowUI = true;
bool gQuitApp = false;
std::mutex m;

void ErrorCallback(int error, const char* description) {
    LOGE("[GLFW]: ", error, " - ", description);
}

void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
    ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);

    if (ImGui::GetIO().WantCaptureMouse || ImGui::IsWindowHovered()) {
        return;
    }
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
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);

    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }

    gScale += 0.01 * (yoffset > 0 ? 1.0 : -1.0);
    if (gScale < 0.1) {
        gScale = 0.1;
    }
}

struct RenderData {
    Mesh mesh;
    glm::vec3 color;
    bool selected = false;
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
    GLFWwindow* window = glfwCreateWindow(WindowWidth, WindowHeight, "VisualDebugger", NULL, NULL);
    if (!window) {
        LOGE("[GLFW]: create window failed");
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    if (gladLoadGL() == 0) {
        LOGF("[GLAD]: glad init failed!");
        return 2;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, false);
    ImGui_ImplOpenGL3_Init("#version 410");

    glfwSetCursorPosCallback(window, CursorPositionCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    glfwSetMouseButtonCallback(window, ImGui_ImplGlfw_MouseButtonCallback);

    Renderer::Init();
    auto& renderer = Renderer::Instance();
    auto& camera = renderer.GetCamera();
    camera.MoveTo(glm::vec3(0, 5, 5));
    camera.Lookat(glm::vec3(0, 0, 0));

    std::thread th([&](){
        auto net = net::Init();
        NetRecv recv(net, 8999);

        bool shouldQuit = false;

        while (!shouldQuit) {
            recv.TryAccept();
            auto packets = recv.RecvPacket();
            std::unique_lock guard(m, std::defer_lock);
            guard.lock();
            for (const auto& packet : packets) {
                std::vector<Vertex> vertices;
                for (const auto& pos : packet.data.positions) {
                    vertices.push_back(Vertex{pos});
                }
                Mesh mesh = Mesh::Create(packet.type, vertices);
                gDatas[packet.data.name] = RenderData{mesh, packet.data.color};
            }
            guard.unlock();

            std::unique_lock l(m, std::defer_lock);
            l.lock();
            shouldQuit = gQuitApp;
            l.unlock();
        }
    });
    th.detach();

    bool show_demo_window = true;
    auto& frustum = renderer.GetCamera().GetFrustum();

    while (!gQuitApp) {
        std::unique_lock l(m, std::defer_lock);
        l.lock();
        gQuitApp = glfwWindowShouldClose(window);
        l.unlock();

        glfwPollEvents();

        double x, y;
        glfwGetCursorPos(window, &x, &y);
        float halfW = std::tan(frustum.fov) * frustum.Near();
        float halfH = halfW / frustum.Aspect();
        Linear ray = Linear::CreateFromLine(glm::vec3(0.0),
                        glm::vec3((x / WindowWidth - 0.5) * 2.0 * halfW,
                        ((WindowHeight - y) / WindowHeight - 0.5) * 2.0 * halfH,
                        -frustum.Near()));

        renderer.Start(glm::vec3(0.2, 0.2, 0.2));

        std::unique_lock lock(m, std::defer_lock);
        lock.lock();
        for (auto& data : gDatas) {
            auto model = glm::rotate(glm::scale(
                    glm::translate(glm::mat4(1.0), -gOrigin),
                    glm::vec3(gScale, gScale, gScale)),
                gRotateY, glm::vec3(0, 1, 0));

            /*
            const auto& mesh = data.second.mesh;
            for (int i = 0; i < mesh.vertices.size(); i++) {
                const auto& pt1 = glm::vec3(camera.View() * model * glm::vec4(mesh.vertices[i].position, 1.0));
                const auto& pt2 = glm::vec3(camera.View() * model * glm::vec4(mesh.vertices[(i + 1) % mesh.vertices.size()].position, 1.0));
                auto seg = Linear::CreateFromSeg(pt1, pt2);
                auto result = RaySegNearest(ray, seg);
                if (result) {
                    std::cout << "t1 = " << result.value().first << ", t2 = " << result.value().second << std::endl;
                    auto p1 = ray.At(result.value().first);
                    auto p2 = seg.At(result.value().second);
                    std::cout << "lengt = " << (p1 - p2).length() << std::endl;
                    if ((p1 - p2).length() <= 0.01) {
                        data.second.selected = true;
                    }
                }
            }
            */

            if (data.second.selected) {
                renderer.SetLineWidth(5);
            } else {
                renderer.SetLineWidth(1);
            }
            renderer.Draw(data.second.mesh, model, data.second.color);
        }
        lock.unlock();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (gShowUI) {
            ImGui::Begin("ui", &gShowUI);
            if (ImGui::Button("clear all")) {
                gDatas.clear();
            }
            ImGui::Text("offset: (%f, %f, %f)", gOrigin.x, gOrigin.y, gOrigin.z);
            ImGui::Text("y rotateion: %f", gRotateY);
            ImGui::Text("scale: %f", gScale);
            for (auto& [name, data] : gDatas) {
                ImGui::Checkbox((name).c_str(), &data.selected);
            }
            ImGui::End();
        }

        ImGui::Render();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    Renderer::Quit();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}