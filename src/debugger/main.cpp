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
float gRotateX = 0;
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
        gRotateY -= (xpos - oldX) * 0.001;
        gRotateX -= (ypos - oldY) * 0.001;
    }

    auto model = glm::rotate(glm::mat4(1.0), -gRotateX, glm::vec3(1.0, 0, 0 )) *
                glm::rotate(glm::mat4(1.0), -gRotateY, glm::vec3(0.0, 1.0, 0.0));

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        gOrigin -= float((xpos - oldX) * 0.01) / gScale * glm::vec3(model * glm::vec4(1, 0, 0, 1));
        gOrigin -= float((ypos - oldY) * 0.01) / gScale * glm::vec3(model * glm::vec4(0, 0, 1, 1));
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
        gOrigin.y += float((ypos - oldY) * 0.01) / gScale;
    }

    oldX = xpos;
    oldY = ypos;
}

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);

    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }

    if (yoffset > 0) {
        gScale /= 0.9;
    } else {
        gScale *= 0.9;
    }
    if (gScale < 0.0001) {
        gScale = 0.0001;
    }
}

struct RenderData {
    Mesh mesh;
    glm::vec3 color;
    bool selected = false;

    // something for ImGui
    std::string checkboxName;
    std::string buttonName;
    
    std::vector<RenderData> childrens;

    RenderData() = default;

    RenderData(const Mesh mesh, glm::vec3 color, std::string name): mesh(mesh), color(color) {
        checkboxName = "##" + name;
        buttonName = "G##" + name;
    }
};

std::unordered_map<std::string, RenderData> gDatas;
std::vector<std::string> gDataNames;

void ImGui_ImplGlfw_SetClipbord(void*, const char* text) {
    glfwSetClipboardString(nullptr, text);
}

const char* ImGui_ImplGlfw_GetClipbord(void*) {
    return glfwGetClipboardString(nullptr);
}

void RunThread(std::unique_ptr<net::Socket>&& client) {
    client->SetNonblock(false);
    NetRecv recv(std::move(client));

    bool shouldQuit = false;

    while (!shouldQuit && recv) {
        auto packets = recv.RecvPacket();
        if (!packets.empty()) {
            std::unique_lock guard(m, std::defer_lock);
            guard.lock();
            for (const auto& packet : packets) {
                std::vector<Vertex> vertices;
                for (const auto& pos : packet.data.positions) {
                    vertices.push_back(Vertex{pos});
                }
                Mesh mesh = Mesh::Create(packet.type, vertices);
                if (auto it = gDatas.find(packet.data.name); it == gDatas.end()) {
                    gDataNames.push_back(packet.data.name);
                }
                gDatas.insert_or_assign(packet.data.name, RenderData(mesh, packet.data.color, packet.data.name));
            }
            guard.unlock();
        }

        std::unique_lock l(m, std::defer_lock);
        l.lock();
        shouldQuit = gQuitApp;
        l.unlock();
    }

}

int main(int argc, char** argv) {
    // if (argc != 2) {
    //     std::cout << "you must give me a port" << std::endl;
    //     return 0;
    // }

    if (!glfwInit()) {
        LOGF("[APP]: glfw init failed");
        return 1;
    }

    glfwSetErrorCallback(ErrorCallback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    GLFWwindow* window = glfwCreateWindow(WindowWidth, WindowHeight, ("VisualDebugger: " + (argc == 2 ? std::string(argv[1]) : std::string("8999"))).c_str(), NULL, NULL);
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
    io.SetClipboardTextFn = ImGui_ImplGlfw_SetClipbord;
    io.GetClipboardTextFn = ImGui_ImplGlfw_GetClipbord;

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

    std::vector<Vertex> vertices;
    for (int x = -20; x <= 20; x++) {
        vertices.push_back(Vertex{glm::vec3(x, 0, -20)});
        vertices.push_back(Vertex{glm::vec3(x, 0, 20)});
    }
    for (int z = -20; z <= 20; z++) {
        vertices.push_back(Vertex{glm::vec3(-20, 0, z)});
        vertices.push_back(Vertex{glm::vec3(20, 0, z)});
    }
    auto gridMesh = Mesh::Create(Mesh::Type::Lines, vertices);

    vertices.clear();
    vertices.push_back(Vertex{glm::vec3(0.0)});
    vertices.push_back(Vertex{glm::vec3(10000.0, 0, 0)});
    auto xAxis= Mesh::Create(Mesh::Type::Lines, vertices);
    vertices.clear();
    vertices.push_back(Vertex{glm::vec3(0.0)});
    vertices.push_back(Vertex{glm::vec3(0, 10000.0, 0)});
    auto yAxis= Mesh::Create(Mesh::Type::Lines, vertices);
    vertices.clear();
    vertices.push_back(Vertex{glm::vec3(0.0)});
    vertices.push_back(Vertex{glm::vec3(0, 0, 10000.0)});
    auto zAxis= Mesh::Create(Mesh::Type::Lines, vertices);




    bool show_demo_window = true;
    auto& frustum = renderer.GetCamera().GetFrustum();

    // init net
    auto net = net::Init();

    const uint32_t port = argc == 2 ? std::atoi(argv[1]) : 8999;

    // create socket
    auto result = net::AddrInfoBuilder::CreateTCP("localhost", port).Build();
    if (result.result != 0) {
        LOGE("create tcp on localhost:", port, " failed!");
        return 1;
    }

    auto socket = net->CreateSocket(result.value, true);
    if (socket->Bind() == SOCKET_ERROR) {
        LOGT("bind failed: ", net::Error2Str(WSAGetLastError()));
        return 1;
    }
    if (socket->Listen(2) != 0) {
        LOGF("listen failed: ", net::Error2Str(WSAGetLastError()));
        return 1;
    }
    LOGI("listening on ", port, "...");
   
    while (!gQuitApp) {
        auto client = socket->Accept();
        if (!client.value) {
            if (client.result != WSAEWOULDBLOCK) {
                std::cerr << "accept failed:" << net::Error2Str(client.result) << std::endl;
            }
        } else {
            LOGI("connected client");
            if (!client.value->Valid()) {
                LOGI("client not valid");
                client.value = nullptr;
            } else {
                std::thread th(RunThread, std::move(client.value));
                th.detach();
            }
        }
        
 
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
        auto model = glm::rotate(glm::mat4(1.0), gRotateX, glm::vec3(1, 0, 0 )) *
                    glm::rotate(glm::mat4(1.0), gRotateY, glm::vec3(0.0, 1.0, 0.0)) *
                    glm::scale(glm::mat4(1.0), glm::vec3(gScale, gScale, gScale)) *
                    glm::translate(glm::mat4(1.0), -gOrigin);
        renderer.SetLineWidth(1);
        renderer.Draw(gridMesh, model, glm::vec3(0.4, 0.4, 0.4));
        renderer.SetLineWidth(3);
        renderer.Draw(xAxis, model, glm::vec3(1, 0, 0));
        renderer.Draw(yAxis, model, glm::vec3(0, 1, 0));
        renderer.Draw(zAxis, model, glm::vec3(0, 0, 1));
        renderer.SetLineWidth(1);

        std::unique_lock lock(m, std::defer_lock);
        lock.lock();
        for (const auto& name : gDataNames) {
            if (auto it = gDatas.find(name); it == gDatas.end()) {
                continue;
            }
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

            auto& data = gDatas[name];
            if (data.selected) {
                renderer.SetLineWidth(5);
            } else {
                renderer.SetLineWidth(1);
            }
            auto color = data.color;
            if (data.mesh.type == Mesh::Type::Points && data.selected) {
                color = glm::vec3(1.0, 1.0, 1.0) - color;
            }
            renderer.Draw(data.mesh, model, color);
        }
        lock.unlock();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        gShowUI = true;
        if (gShowUI) {
            ImGui::Begin("ui", &gShowUI);
            if (ImGui::Button("clear all")) {
                gDatas.clear();
                gDataNames.clear();
            }
            ImGui::Text("offset: (%f, %f, %f)", gOrigin.x, gOrigin.y, gOrigin.z);
            ImGui::Text("x rotateion: %f", gRotateX);
            ImGui::Text("y rotateion: %f", gRotateY);
            ImGui::Text("scale: %f", gScale);
            for (const auto& name : gDataNames) {
                if (auto it = gDatas.find(name); it == gDatas.end()) {
                    continue;
                }
                auto& data = gDatas[name];
                ImGui::Checkbox(data.checkboxName.c_str(), &data.selected);
                ImGui::SameLine();
                if (ImGui::Button(data.buttonName.c_str())) {
                    gOrigin = data.mesh.vertices[0].position;
                }
                ImGui::SameLine();
                if (ImGui::CollapsingHeader(name.c_str())) {
                    for (int i = 0; i < data.mesh.vertices.size(); i++) {
                        const auto& vertex = data.mesh.vertices[i];
                        static char buf[1024] = {0};
                        snprintf(buf, sizeof(buf), "[%d]: (%f, %f, %f)", i, vertex.position.x, vertex.position.y, vertex.position.z); 
                        if (ImGui::Button(buf)) {
                            glfwSetClipboardString(window, buf);
                        }
                    }
                }
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

    socket->Close();
    Renderer::Quit();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}