#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <memory>

#include "Camera.h"
#include "Shader.h"
#include "Terrain.h"
#include "Skybox.h"
#include "Tree.h"

// Window settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Camera - 电子初始位置以更好地查看火山
Camera camera(glm::vec3(60.0f, 40.0f, 60.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

int main() {
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Realistic Volcano Terrain", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Capture mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Configure OpenGL
    glEnable(GL_DEPTH_TEST);

    // Build and compile shaders
    Shader terrainShader("shaders/terrain.vert", "shaders/terrain.frag");
    Shader skyboxShader("shaders/skybox.vert", "shaders/skybox.frag");
    Shader treeShader("shaders/tree.vert", "shaders/tree.frag");
 
    // Create terrain - 缩小的地形尺寸，火山口稍小
    Terrain terrain(150, 1.0f, 45.0f, 10.0f);  // 火山口半径调整为10
    auto terrainMesh = terrain.GenerateMesh();

    // Create skybox
    Skybox skybox;

    // 创建树木系统
    Tree trees;
    // 生成树木实例，参数：数量，地形大小，火山半径
    trees.GenerateInstances(500, 150.0f, 45.0f, &terrain);  // 生成500棵树

    // Light position
    glm::vec3 lightPos(150.0f, 150.0f, 150.0f);

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        // Per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Input
        processInput(window);

        // Render
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // View/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f, 1000.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);

        // 1. 渲染地形
        terrainShader.use();
        terrainShader.setMat4("projection", projection);
        terrainShader.setMat4("view", view);
        terrainShader.setMat4("model", model);
        terrainShader.setVec3("lightPos", lightPos);
        terrainShader.setVec3("viewPos", camera.Position);
        terrainShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
        terrainMesh->Draw();

        // 2. 渲染树木
        treeShader.use();
        treeShader.setMat4("projection", projection);
        treeShader.setMat4("view", view);
        treeShader.setVec3("lightPos", lightPos);
        treeShader.setVec3("viewPos", camera.Position);
        treeShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
        trees.DrawInstanced(treeShader);

        // 3. 最后渲染天空盒（确保天空盒在最后面）
        glDepthFunc(GL_LEQUAL);  // 改变深度函数，使天空盒通过深度测试
        skyboxShader.use();

        // 移除视图矩阵的平移部分
        glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
        skyboxShader.setMat4("projection", projection);
        skyboxShader.setMat4("view", skyboxView);

        skybox.Draw(skyboxShader);
        glDepthFunc(GL_LESS);  // 恢复默认深度函数

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Terminate
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // 增加移动速度以适应更大的地形
    float cameraSpeed = 2.5f;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraSpeed = 5.0f;  // 按住Shift加速

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime * cameraSpeed);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime * cameraSpeed);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime * cameraSpeed);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime * cameraSpeed);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}