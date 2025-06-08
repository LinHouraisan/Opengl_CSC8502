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
#include "LavaLake.h"
#include "LavaFlow.h"

// Window settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Shadow map settings
const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;

// Camera
Camera camera(glm::vec3(60.0f, 40.0f, 60.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Day/Night cycle
float timeOfDay = 0.0f; // 0.0 = noon, 0.5 = sunset/sunrise, 1.0 = midnight
bool isTransitioning = false;
float transitionSpeed = 0.5f; // 过渡速度
float targetTimeOfDay = 0.0f;

// Callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// Shadow mapping functions
unsigned int createShadowMap();
glm::mat4 calculateLightSpaceMatrix(const glm::vec3& lightPos, const glm::vec3& targetPos);
glm::vec3 calculateSunPosition(float timeOfDay);
glm::vec3 calculateLightColor(float timeOfDay);

int main() {
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Realistic Volcano with Day/Night Cycle", NULL, NULL);
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
    Shader shadowShader("shaders/shadow.vert", "shaders/shadow.frag");
    Shader shadowInstanceShader("shaders/shadow_instance.vert", "shaders/shadow.frag");
    Shader lavaShader("shaders/lava.vert", "shaders/lava.frag");
    Shader lavaFlowShader("shaders/lava_flow.vert", "shaders/lava_flow.frag");

    // Create terrain
    Terrain terrain(150, 1.0f, 45.0f, 10.0f);
    auto terrainMesh = terrain.GenerateMesh();

    float craterHeight = terrain.GetHeightAt(0.0f, 0.0f);
    std::cout << "Crater height at center: " << craterHeight << std::endl;

    // Create lava lake and position it at the crater
    LavaLake lavaLake(8.0f, 64);
    // 将岩浆湖放置在火山口底部，稍微高一点避免z-fighting
    lavaLake.SetPosition(glm::vec3(0.0f, craterHeight + 2.0f, 0.0f));

    // Create lava flow system
    LavaFlow lavaFlow(&terrain, 0.8f);
    lavaFlow.StartFlow();

    // Store lava flow pointer in window user pointer for input handling
    glfwSetWindowUserPointer(window, &lavaFlow);

    // Create skybox
    Skybox skybox;

    // Create trees
    Tree trees;
    trees.GenerateInstances(500, 150.0f, 45.0f, &terrain);

    // Create shadow map
    unsigned int depthMapFBO = createShadowMap();
    unsigned int depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Configure shadow samplers
    terrainShader.use();
    terrainShader.setInt("shadowMap", 1);
    treeShader.use();
    treeShader.setInt("shadowMap", 1);

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        // Per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Update day/night cycle
        if (isTransitioning) {
            float diff = targetTimeOfDay - timeOfDay;
            if (abs(diff) < 0.01f) {
                timeOfDay = targetTimeOfDay;
                isTransitioning = false;
            }
            else {
                timeOfDay += diff * transitionSpeed * deltaTime;
            }

            // Keep timeOfDay in [0, 1] range
            if (timeOfDay > 1.0f) timeOfDay -= 1.0f;
            if (timeOfDay < 0.0f) timeOfDay += 1.0f;
        }

        // Update lava flow
        lavaFlow.Update(deltaTime);

        // Input
        processInput(window);

        // Calculate sun position and light properties
        glm::vec3 sunPos = calculateSunPosition(timeOfDay);
        glm::vec3 lightColor = calculateLightColor(timeOfDay);
        glm::vec3 targetPos(0.0f, 0.0f, 0.0f); // Center of terrain
        glm::mat4 lightSpaceMatrix = calculateLightSpaceMatrix(sunPos, targetPos);

        // 1. Render depth map (shadows)
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        // Render terrain to depth map
        shadowShader.use();
        shadowShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
        shadowShader.setMat4("model", glm::mat4(1.0f));
        terrainMesh->Draw();

        // Render trees to depth map
        shadowInstanceShader.use();
        shadowInstanceShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
        trees.DrawInstanced(shadowInstanceShader);

        // 2. Render scene normally
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // View/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);

        // Bind shadow map
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);

        // Render terrain
        terrainShader.use();
        terrainShader.setMat4("projection", projection);
        terrainShader.setMat4("view", view);
        terrainShader.setMat4("model", model);
        terrainShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
        terrainShader.setVec3("lightPos", sunPos);
        terrainShader.setVec3("viewPos", camera.Position);
        terrainShader.setVec3("lightColor", lightColor);
        terrainShader.setFloat("shadowIntensity", lightColor.r > 0.5f ? 0.8f : 0.3f);
        terrainShader.setVec3("lavaPos", glm::vec3(0.0f, -7.5f, 0.0f));
        terrainShader.setVec3("lavaColor", glm::vec3(1.0f, 0.3f, 0.0f));
        terrainShader.setFloat("time", currentFrame);  // 用于脉动效果
        terrainMesh->Draw();

        // Render lava lake
        glDepthFunc(GL_LEQUAL);  // 允许深度相等的片段通过
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // 使用标准透明混合

        lavaShader.use();
        lavaShader.setMat4("projection", projection);
        lavaShader.setMat4("view", view);
        lavaShader.setVec3("viewPos", camera.Position);
        lavaShader.setFloat("time", currentFrame);
        lavaLake.Draw(lavaShader, currentFrame);

        glDisable(GL_BLEND);
        glDepthFunc(GL_LESS);  // 恢复正常深度测试

        // 更新地形着色器的岩浆位置
        glm::vec3 lavaPos = lavaLake.GetPosition();
        terrainShader.setVec3("lavaPos", lavaPos);
        treeShader.setVec3("lavaPos", lavaPos);

        // Render lava flow particles
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);  // 加法混合，让岩浆发光

        lavaFlowShader.use();
        lavaFlowShader.setMat4("projection", projection);
        lavaFlowShader.setMat4("view", view);
        lavaFlowShader.setVec3("viewPos", camera.Position);
        lavaFlowShader.setFloat("time", currentFrame);
        lavaFlow.Draw(lavaFlowShader, currentFrame);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);

        // Render trees
        treeShader.use();
        treeShader.setMat4("projection", projection);
        treeShader.setMat4("view", view);
        treeShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
        treeShader.setVec3("lightPos", sunPos);
        treeShader.setVec3("viewPos", camera.Position);
        treeShader.setVec3("lightColor", lightColor);
        treeShader.setFloat("shadowIntensity", lightColor.r > 0.5f ? 0.8f : 0.3f);
        terrainShader.setVec3("lavaPos", glm::vec3(0.0f, -7.5f, 0.0f));
        terrainShader.setVec3("lavaColor", glm::vec3(1.0f, 0.3f, 0.0f));
        terrainShader.setFloat("time", currentFrame);  // 用于脉动效果
        trees.DrawInstanced(treeShader);

        // Render skybox last
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
        skyboxShader.setMat4("projection", projection);
        skyboxShader.setMat4("view", skyboxView);
        skyboxShader.setFloat("timeOfDay", timeOfDay);
        skyboxShader.setVec3("sunDirection", glm::normalize(sunPos));
        skybox.Draw(skyboxShader);
        glDepthFunc(GL_LESS);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteFramebuffers(1, &depthMapFBO);
    glDeleteTextures(1, &depthMap);
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Camera movement
    float cameraSpeed = 2.5f;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraSpeed = 5.0f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime * cameraSpeed);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime * cameraSpeed);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime * cameraSpeed);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime * cameraSpeed);

    // Day/Night cycle control
    static bool tKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !tKeyPressed) {
        tKeyPressed = true;
        isTransitioning = true;

        // Toggle between day and night
        if (timeOfDay < 0.25f || timeOfDay > 0.75f) {
            // Currently day, transition to night
            targetTimeOfDay = 0.5f;
        }
        else {
            // Currently night, transition to day
            targetTimeOfDay = 0.0f;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE) {
        tKeyPressed = false;
    }

    // Lava flow control
    static bool yKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS && !yKeyPressed) {
        yKeyPressed = true;
        LavaFlow* lavaFlow = static_cast<LavaFlow*>(glfwGetWindowUserPointer(window));
        if (lavaFlow) {
            if (lavaFlow->IsFlowing()) {
                lavaFlow->StopFlow();
            }
            else {
                lavaFlow->StartFlow();
            }
        }
    }
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_RELEASE) {
        yKeyPressed = false;
    }

    // Manual time control
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        timeOfDay = 0.0f; // Noon
        isTransitioning = false;
    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        timeOfDay = 0.25f; // Afternoon
        isTransitioning = false;
    }
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
        timeOfDay = 0.5f; // Sunset
        isTransitioning = false;
    }
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
        timeOfDay = 0.75f; // Night
        isTransitioning = false;
    }
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
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}

unsigned int createShadowMap() {
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    return depthMapFBO;
}

glm::mat4 calculateLightSpaceMatrix(const glm::vec3& lightPos, const glm::vec3& targetPos) {
    // 使用正交投影创建光源空间矩阵
    glm::mat4 lightProjection = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, 1.0f, 200.0f);
    glm::mat4 lightView = glm::lookAt(lightPos, targetPos, glm::vec3(0.0f, 1.0f, 0.0f));
    return lightProjection * lightView;
}

glm::vec3 calculateSunPosition(float timeOfDay) {
    // 计算太阳角度 (0 = 正午90度, 0.5 = 日落/日出0度, 1.0 = 午夜-90度)
    float angle = (timeOfDay * 2.0f - 0.5f) * 3.14159f; // -π/2 to 3π/2

    float distance = 150.0f;
    float x = distance * cos(angle);
    float y = distance * sin(angle);

    // 太阳从东升西落
    return glm::vec3(x, y, 0.0f);
}

glm::vec3 calculateLightColor(float timeOfDay) {
    // 根据时间计算光照颜色和强度
    glm::vec3 color;

    if (timeOfDay < 0.25f) {
        // 白天 - 明亮的白光
        color = glm::vec3(1.0f, 1.0f, 1.0f);
    }
    else if (timeOfDay < 0.5f) {
        // 日落 - 橙红色渐变
        float t = (timeOfDay - 0.25f) * 4.0f;
        color = glm::mix(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 0.6f, 0.3f), t);
    }
    else if (timeOfDay < 0.75f) {
        // 夜晚 - 微弱的蓝色月光
        float t = (timeOfDay - 0.5f) * 4.0f;
        color = glm::mix(glm::vec3(1.0f, 0.6f, 0.3f), glm::vec3(0.2f, 0.2f, 0.4f), t);
    }
    else {
        // 日出 - 从蓝色过渡到白色
        float t = (timeOfDay - 0.75f) * 4.0f;
        color = glm::mix(glm::vec3(0.2f, 0.2f, 0.4f), glm::vec3(1.0f, 1.0f, 1.0f), t);
    }

    return color;
}