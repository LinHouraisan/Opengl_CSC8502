#include "LavaBubbles.h"
#include <iostream>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

LavaBubbles::LavaBubbles(const glm::vec3& lakeCenter, float lakeRadius, int maxBubbles)
    : lakeCenter(lakeCenter), lakeRadius(lakeRadius), maxBubbles(maxBubbles),
    spawnTimer(0.0f), spawnRate(3.0f),  // 每秒3个气泡
    rng(std::random_device{}()),
    radiusDist(0.0f, 0.8f),  // 在湖的80%半径内生成
    sizeDist(0.3f, 1.2f),    // 气泡大小变化
    lifeDist(2.0f, 5.0f),    // 生命周期2-5秒
    angleDist(0.0f, 6.28318f) {  // 0-2π

    bubbles.resize(maxBubbles);
    for (auto& bubble : bubbles) {
        bubble.active = false;
    }

    setupMesh();
}

LavaBubbles::~LavaBubbles() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &instanceVBO);
}

void LavaBubbles::Update(float deltaTime) {
    // 生成新气泡
    spawnTimer += deltaTime;
    float spawnInterval = 1.0f / spawnRate;

    while (spawnTimer >= spawnInterval) {
        spawnBubble();
        spawnTimer -= spawnInterval;
    }

    // 更新现有气泡
    for (auto& bubble : bubbles) {
        if (bubble.active) {
            updateBubble(bubble, deltaTime);
        }
    }

    // 更新实例缓冲区
    updateInstanceBuffer();
}

void LavaBubbles::spawnBubble() {
    // 找到一个非活跃的气泡
    for (auto& bubble : bubbles) {
        if (!bubble.active) {
            // 在岩浆湖内随机位置生成
            float r = radiusDist(rng) * lakeRadius;
            float angle = angleDist(rng);

            bubble.position = lakeCenter + glm::vec3(
                r * cos(angle),
                0.0f,  // 从湖底开始
                r * sin(angle)
            );

            // 初始向上速度，带一点随机水平偏移
            bubble.velocity = glm::vec3(
                (angleDist(rng) - 3.14159f) * 0.1f,  // 小的水平速度
                1.5f + sizeDist(rng) * 0.5f,         // 向上速度
                (angleDist(rng) - 3.14159f) * 0.1f
            );

            bubble.size = sizeDist(rng);
            bubble.lifetime = 0.0f;
            bubble.maxLifetime = lifeDist(rng);
            bubble.wobble = angleDist(rng);  // 随机初始相位
            bubble.active = true;

            break;
        }
    }
}

void LavaBubbles::updateBubble(Bubble& bubble, float deltaTime) {
    bubble.lifetime += deltaTime;

    // 检查是否超过生命周期
    if (bubble.lifetime >= bubble.maxLifetime) {
        bubble.active = false;
        return;
    }

    // 生命周期进度 (0-1)
    float lifeProgress = bubble.lifetime / bubble.maxLifetime;

    // 气泡上升时逐渐加速
    bubble.velocity.y += deltaTime * 0.5f;

    // 添加左右摇摆效果
    bubble.wobble += deltaTime * 3.0f;
    float wobbleAmount = sin(bubble.wobble) * 0.2f;

    // 更新位置
    glm::vec3 movement = bubble.velocity * deltaTime;
    movement.x += wobbleAmount * deltaTime;
    bubble.position += movement;

    // 气泡在上升过程中会逐渐变大
    float sizeMultiplier = 1.0f + lifeProgress * 0.5f;
    bubble.size = bubble.size * sizeMultiplier * (1.0f - lifeProgress * 0.3f);

    // 如果气泡上升到湖面以上一定高度，让它破裂
    if (bubble.position.y > lakeCenter.y + 2.0f) {
        bubble.active = false;
    }
}

void LavaBubbles::updateInstanceBuffer() {
    activeTransforms.clear();

    for (const auto& bubble : bubbles) {
        if (bubble.active) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, bubble.position);

            // 根据生命周期调整大小
            float lifeProgress = bubble.lifetime / bubble.maxLifetime;
            float scale = bubble.size * (1.0f + lifeProgress * 0.3f);

            // 在接近消失时快速缩小
            if (lifeProgress > 0.8f) {
                float fadeProgress = (lifeProgress - 0.8f) / 0.2f;
                scale *= (1.0f - fadeProgress);
            }

            model = glm::scale(model, glm::vec3(scale));
            activeTransforms.push_back(model);
        }
    }

    // 更新GPU缓冲区
    if (!activeTransforms.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, activeTransforms.size() * sizeof(glm::mat4),
            activeTransforms.data(), GL_DYNAMIC_DRAW);
    }
}

void LavaBubbles::Draw(Shader& shader, float time) {
    if (activeTransforms.empty()) return;

    shader.setFloat("time", time);

    glBindVertexArray(VAO);
    glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0,
        activeTransforms.size());
    glBindVertexArray(0);
}

void LavaBubbles::setupMesh() {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // 创建一个球体网格
    const int latSegments = 12;
    const int lonSegments = 12;
    const float radius = 1.0f;

    // 生成顶点
    for (int lat = 0; lat <= latSegments; lat++) {
        float theta = lat * 3.14159f / latSegments;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);

        for (int lon = 0; lon <= lonSegments; lon++) {
            float phi = lon * 2.0f * 3.14159f / lonSegments;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);

            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;

            // 位置
            vertices.push_back(x * radius);
            vertices.push_back(y * radius);
            vertices.push_back(z * radius);
            // 法线
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            // 纹理坐标
            vertices.push_back((float)lon / lonSegments);
            vertices.push_back((float)lat / latSegments);
        }
    }

    // 生成索引
    for (int lat = 0; lat < latSegments; lat++) {
        for (int lon = 0; lon < lonSegments; lon++) {
            int first = lat * (lonSegments + 1) + lon;
            int second = first + lonSegments + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    indexCount = indices.size();

    // 创建VAO/VBO/EBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glGenBuffers(1, &instanceVBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
        vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
        indices.data(), GL_STATIC_DRAW);

    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 法线属性
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
        (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // 纹理坐标属性
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
        (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // 实例矩阵属性
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    for (int i = 0; i < 4; i++) {
        glEnableVertexAttribArray(3 + i);
        glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
            (void*)(i * sizeof(glm::vec4)));
        glVertexAttribDivisor(3 + i, 1);
    }

    glBindVertexArray(0);
}