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
#include "LavaBubbles.h"
#include "VolcanicAsh.h"

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
float timeOfDay = 0.0f;
bool isTransitioning = false;
float transitionSpeed = 0.5f;
float targetTimeOfDay = 0.0f;

// Wind parameters
glm::vec3 windDirection(1.0f, 0.0f, 0.5f);
float windStrength = 3.0f;

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

// 存储系统指针的结构体
struct SystemPointers {
    LavaFlow* lavaFlow;
    VolcanicAsh* volcanicAsh;
};

int main() {
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Realistic Volcano with Dense Ash Cloud", NULL, NULL);
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
    Shader bubbleShader("shaders/bubble.vert", "shaders/bubble.frag");
    Shader ashShader("shaders/ash.vert", "shaders/ash.frag");

    // Create terrain
    Terrain terrain(150, 1.0f, 45.0f, 10.0f);
    auto terrainMesh = terrain.GenerateMesh();

    float craterHeight = terrain.GetHeightAt(0.0f, 0.0f);
    std::cout << "Crater height at center: " << craterHeight << std::endl;

    // Create lava lake and position it at the crater
    LavaLake lavaLake(8.0f, 64);
    lavaLake.SetPosition(glm::vec3(0.0f, craterHeight + 2.0f, 0.0f));

    // Create lava bubbles system
    LavaBubbles lavaBubbles(lavaLake.GetPosition(), 8.0f, 40);

    // Create enhanced lava flow system
    LavaFlow lavaFlow(&terrain, 0.8f);
    lavaFlow.SetLakeCenter(lavaLake.GetPosition());
    lavaFlow.StartFlow();

    // Create volcanic ash system - 调整参数
    glm::vec3 ashEmissionCenter = glm::vec3(0.0f, craterHeight + 10.0f, 0.0f);  // 提高发射高度
    VolcanicAsh volcanicAsh(ashEmissionCenter, 3.0f, 1000);  // 减小发射半径，增加粒子数
    volcanicAsh.SetWind(windDirection, windStrength);
    volcanicAsh.SetIntensity(2.0f);  // 提高初始强度

    // Store system pointers for input handling
    SystemPointers systems = { &lavaFlow, &volcanicAsh };
    glfwSetWindowUserPointer(window, &systems);

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

    // Print controls
    std::cout << "\n=== VOLCANO ERUPTION CONTROLS ===" << std::endl;
    std::cout << "WASD - Move camera" << std::endl;
    std::cout << "Mouse - Look around" << std::endl;
    std::cout << "Shift - Move faster" << std::endl;
    std::cout << "T - Toggle day/night cycle" << std::endl;
    std::cout << "Y - Toggle lava flow on/off" << std::endl;
    std::cout << "E - Trigger volcanic eruption!" << std::endl;
    std::cout << "A - Toggle volcanic ash emission" << std::endl;
    std::cout << "1-4 - Set time of day (noon/afternoon/sunset/night)" << std::endl;
    std::cout << "Arrow Keys - Control wind direction" << std::endl;
    std::cout << "Page Up/Down - Control wind strength" << std::endl;
    std::cout << "================================\n" << std::endl;

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

        // Update systems
        lavaFlow.Update(deltaTime);
        lavaBubbles.Update(deltaTime);
        volcanicAsh.Update(deltaTime);

        // Update wind for ash
        volcanicAsh.SetWind(windDirection, windStrength);

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
        terrainShader.setFloat("time", currentFrame);
        terrainMesh->Draw();

        // Render lava lake
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        lavaShader.use();
        lavaShader.setMat4("projection", projection);
        lavaShader.setMat4("view", view);
        lavaShader.setVec3("viewPos", camera.Position);
        lavaShader.setFloat("time", currentFrame);
        lavaLake.Draw(lavaShader, currentFrame);

        glDisable(GL_BLEND);
        glDepthFunc(GL_LESS);

        // Render lava bubbles
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        bubbleShader.use();
        bubbleShader.setMat4("projection", projection);
        bubbleShader.setMat4("view", view);
        bubbleShader.setVec3("viewPos", camera.Position);
        bubbleShader.setFloat("time", currentFrame);
        lavaBubbles.Draw(bubbleShader, currentFrame);

        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);

        // Update terrain shader's lava position
        glm::vec3 lavaPos = lavaLake.GetPosition();
        terrainShader.setVec3("lavaPos", lavaPos);
        treeShader.setVec3("lavaPos", lavaPos);

        // Render lava flow particles
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);  // Additive blending for glow

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
        trees.DrawInstanced(treeShader);

        // Render volcanic ash
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);  // 禁用深度写入，但仍然进行深度测试

        ashShader.use();
        ashShader.setMat4("projection", projection);
        ashShader.setMat4("view", view);
        ashShader.setVec3("viewPos", camera.Position);
        ashShader.setFloat("time", currentFrame);
        volcanicAsh.Draw(ashShader, currentFrame);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

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

    // Get system pointers
    SystemPointers* systems = static_cast<SystemPointers*>(glfwGetWindowUserPointer(window));

    // Day/Night cycle control
    static bool tKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !tKeyPressed) {
        tKeyPressed = true;
        isTransitioning = true;

        if (timeOfDay < 0.25f || timeOfDay > 0.75f) {
            targetTimeOfDay = 0.5f;
        }
        else {
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
        if (systems && systems->lavaFlow) {
            if (systems->lavaFlow->IsFlowing()) {
                systems->lavaFlow->StopFlow();
            }
            else {
                systems->lavaFlow->StartFlow();
            }
        }
    }
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_RELEASE) {
        yKeyPressed = false;
    }

    // Volcanic eruption control
    static bool eKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS && !eKeyPressed) {
        eKeyPressed = true;
        if (systems) {
            if (systems->lavaFlow) {
                systems->lavaFlow->TriggerEruption(5.0f);
            }
            if (systems->volcanicAsh) {
                systems->volcanicAsh->StartEmission();
                systems->volcanicAsh->SetIntensity(3.0f);  // 大幅增加喷发强度
            }
            std::cout << "VOLCANIC ERUPTION TRIGGERED!" << std::endl;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_RELEASE) {
        eKeyPressed = false;
    }

    // Volcanic ash control
    static bool aKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS && !aKeyPressed) {
        aKeyPressed = true;
        if (systems && systems->volcanicAsh) {
            if (systems->volcanicAsh->IsEmitting()) {
                systems->volcanicAsh->StopEmission();
            }
            else {
                systems->volcanicAsh->StartEmission();
                systems->volcanicAsh->SetIntensity(2.0f);  // 提高正常强度
            }
        }
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_RELEASE) {
        aKeyPressed = false;
    }

    // Wind control
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        float angle = atan2(windDirection.z, windDirection.x);
        angle -= deltaTime;
        windDirection.x = cos(angle);
        windDirection.z = sin(angle);
        std::cout << "Wind direction: (" << windDirection.x << ", " << windDirection.z << ")" << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        float angle = atan2(windDirection.z, windDirection.x);
        angle += deltaTime;
        windDirection.x = cos(angle);
        windDirection.z = sin(angle);
        std::cout << "Wind direction: (" << windDirection.x << ", " << windDirection.z << ")" << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS) {
        windStrength = std::min(windStrength + deltaTime * 5.0f, 20.0f);
        std::cout << "Wind strength: " << windStrength << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) {
        windStrength = std::max(windStrength - deltaTime * 5.0f, 0.0f);
        std::cout << "Wind strength: " << windStrength << std::endl;
    }

    // Manual time control
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        timeOfDay = 0.0f;
        isTransitioning = false;
    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        timeOfDay = 0.25f;
        isTransitioning = false;
    }
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
        timeOfDay = 0.5f;
        isTransitioning = false;
    }
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
        timeOfDay = 0.75f;
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
    glm::mat4 lightProjection = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, 1.0f, 200.0f);
    glm::mat4 lightView = glm::lookAt(lightPos, targetPos, glm::vec3(0.0f, 1.0f, 0.0f));
    return lightProjection * lightView;
}

glm::vec3 calculateSunPosition(float timeOfDay) {
    float angle = (timeOfDay * 2.0f - 0.5f) * 3.14159f;
    float distance = 150.0f;
    float x = distance * cos(angle);
    float y = distance * sin(angle);
    return glm::vec3(x, y, 0.0f);
}

glm::vec3 calculateLightColor(float timeOfDay) {
    glm::vec3 color;

    if (timeOfDay < 0.25f) {
        color = glm::vec3(1.0f, 1.0f, 1.0f);
    }
    else if (timeOfDay < 0.5f) {
        float t = (timeOfDay - 0.25f) * 4.0f;
        color = glm::mix(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 0.6f, 0.3f), t);
    }
    else if (timeOfDay < 0.75f) {
        float t = (timeOfDay - 0.5f) * 4.0f;
        color = glm::mix(glm::vec3(1.0f, 0.6f, 0.3f), glm::vec3(0.2f, 0.2f, 0.4f), t);
    }
    else {
        float t = (timeOfDay - 0.75f) * 4.0f;
        color = glm::mix(glm::vec3(0.2f, 0.2f, 0.4f), glm::vec3(1.0f, 1.0f, 1.0f), t);
    }

    return color;
}