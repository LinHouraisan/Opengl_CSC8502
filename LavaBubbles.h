#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <random>
#include "Shader.h"

struct Bubble {
    glm::vec3 position;
    glm::vec3 velocity;
    float size;
    float lifetime;
    float maxLifetime;
    bool active;
    float wobble;  // 用于左右摇摆
};

class LavaBubbles {
public:
    LavaBubbles(const glm::vec3& lakeCenter, float lakeRadius, int maxBubbles = 50);
    ~LavaBubbles();

    void Update(float deltaTime);
    void Draw(Shader& shader, float time);

    void SetLakePosition(const glm::vec3& pos) { lakeCenter = pos; }

private:
    glm::vec3 lakeCenter;
    float lakeRadius;
    int maxBubbles;
    float spawnTimer;
    float spawnRate;

    std::vector<Bubble> bubbles;
    std::vector<glm::mat4> activeTransforms;

    // 渲染资源
    unsigned int VAO, VBO, EBO;
    unsigned int instanceVBO;
    unsigned int indexCount;

    // 随机数生成
    std::mt19937 rng;
    std::uniform_real_distribution<float> radiusDist;
    std::uniform_real_distribution<float> sizeDist;
    std::uniform_real_distribution<float> lifeDist;
    std::uniform_real_distribution<float> angleDist;

    void setupMesh();
    void spawnBubble();
    void updateBubble(Bubble& bubble, float deltaTime);
    void updateInstanceBuffer();
};