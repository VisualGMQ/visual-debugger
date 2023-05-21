#include "pch.hpp"
#include "renderer.hpp"

void ErrorCallback(int error, const char* description) {
    LOGE("[GLFW]: ", error, " - ", description);
}

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

    Mesh mesh = Mesh::CreateWithIndices(Mesh::Type::Triangles,
        {
            Vertex{glm::vec3( 0.5f,  0.5f, 0.0f)},
            Vertex{glm::vec3( 0.5f, -0.5f, 0.0f)},
            Vertex{glm::vec3(-0.5f, -0.5f, 0.0f)},
            Vertex{glm::vec3(-0.5f,  0.5f, 0.0f)},
        },
        {
            0, 1, 3,
            1, 2, 3,
        });

    Renderer::Init();
    auto& renderer = Renderer::Instance();
    auto& camera = renderer.GetCamera();

    while (!glfwWindowShouldClose(window)) {
        renderer.Start(glm::vec3(0.2, 0.2, 0.2));

        renderer.Draw(mesh, glm::translate(glm::mat4(1.0), glm::vec3(0.0, 0.0, -3.0f)), glm::vec3(0.0, 1.0, 0.0));

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    Renderer::Quit();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}