#include "LavaFlow.h"
#include <algorithm>
#include <random>
#include <iostream>
#include <cstring>  // for memcpy
#include <glm/ext/matrix_transform.hpp>

LavaFlow::LavaFlow(Terrain* terrain, float particleSize)
    : terrain(terrain), particleSize(particleSize), isFlowing(false),
    flowRate(10.0f), emissionTimer(0.0f), maxParticles(2000) {

    // 设置发射中心在火山口
    emissionCenter = glm::vec3(0.0f, terrain->GetHeightAt(0.0f, 0.0f) + 2.0f, 0.0f);
    emissionRadius = 5.0f;

    // 初始化粒子池
    particles.resize(maxParticles);
    for (auto& p : particles) {
        p.active = false;
    }

    setupMesh();
}

LavaFlow::~LavaFlow() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &instanceVBO);
}

void LavaFlow::StartFlow() {
    isFlowing = true;
    std::cout << "Lava flow started!" << std::endl;
}

void LavaFlow::StopFlow() {
    isFlowing = false;
    std::cout << "Lava flow stopped!" << std::endl;
}

void LavaFlow::Update(float deltaTime) {
    // 发射新粒子
    if (isFlowing) {
        emissionTimer += deltaTime;
        float emissionInterval = 1.0f / flowRate;

        while (emissionTimer >= emissionInterval) {
            emitParticle();
            emissionTimer -= emissionInterval;
        }
    }

    // 更新所有活跃粒子
    for (auto& particle : particles) {
        if (particle.active) {
            updateParticle(particle, deltaTime);
        }
    }

    // 更新实例缓冲区
    updateInstanceBuffer();
}

void LavaFlow::emitParticle() {
    // 找到一个非活跃粒子
    for (auto& particle : particles) {
        if (!particle.active) {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);
            static std::uniform_real_distribution<float> radiusDist(0.0f, 1.0f);
            static std::uniform_real_distribution<float> speedDist(0.5f, 2.0f);

            // 在火山口附近随机位置生成
            float angle = angleDist(gen);
            float r = radiusDist(gen) * emissionRadius;

            particle.position = emissionCenter + glm::vec3(
                r * cos(angle),
                0.0f,
                r * sin(angle)
            );

            // 初始速度：稍微向外和向下
            float speed = speedDist(gen);
            particle.velocity = glm::vec3(
                speed * cos(angle),
                -1.0f,
                speed * sin(angle)
            );

            particle.temperature = 1.0f;
            particle.lifetime = 0.0f;
            particle.active = true;

            break;
        }
    }
}

void LavaFlow::updateParticle(LavaParticle& particle, float deltaTime) {
    // 更新生命周期
    particle.lifetime += deltaTime;

    // 温度随时间降低
    particle.temperature = std::max(0.0f, 1.0f - particle.lifetime * 0.05f);

    // 应用重力
    particle.velocity.y -= 9.8f * deltaTime;

    // 获取当前位置的地形高度
    float terrainHeight = terrain->GetHeightAt(particle.position.x, particle.position.z);

    // 更新位置
    glm::vec3 newPos = particle.position + particle.velocity * deltaTime;

    // 地形碰撞检测
    if (newPos.y <= terrainHeight + particleSize * 0.5f) {
        newPos.y = terrainHeight + particleSize * 0.5f;

        // 计算地形法线来确定流动方向
        glm::vec3 normal = terrain->calculateNormal(newPos.x, newPos.z);

        // 将速度投影到地形表面
        float dotProduct = glm::dot(particle.velocity, normal);
        particle.velocity = particle.velocity - normal * dotProduct;

        // 添加沿坡面向下的力
        glm::vec3 downSlope = glm::vec3(0.0f, -1.0f, 0.0f) - normal * glm::dot(glm::vec3(0.0f, -1.0f, 0.0f), normal);
        downSlope = glm::normalize(downSlope);
        particle.velocity += downSlope * 5.0f * deltaTime;

        // 应用摩擦力
        particle.velocity *= 0.95f;

        // 限制最大速度
        float speed = glm::length(particle.velocity);
        if (speed > 10.0f) {
            particle.velocity = (particle.velocity / speed) * 10.0f;
        }
    }

    particle.position = newPos;

    // 停止条件
    // 1. 温度太低（冷却）
    // 2. 速度太慢
    // 3. 到达平原（高度接近0）
    // 4. 超出生命周期
    float speed = glm::length(particle.velocity);
    if (particle.temperature <= 0.1f ||
        speed < 0.1f ||
        terrainHeight < 2.0f ||  // 在平原附近停止
        particle.lifetime > 60.0f) {
        particle.active = false;
    }
}

void LavaFlow::updateInstanceBuffer() {
    activeTransforms.clear();

    for (const auto& particle : particles) {
        if (particle.active) {
            // 创建变换矩阵
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, particle.position);

            // 根据温度调整大小
            float scale = particleSize * (0.5f + 0.5f * particle.temperature);
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

void LavaFlow::Draw(Shader& shader, float time) {
    if (activeTransforms.empty()) return;

    shader.setFloat("time", time);

    glBindVertexArray(VAO);
    glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0, activeTransforms.size());
    glBindVertexArray(0);
}

void LavaFlow::setupMesh() {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // 创建低多边形球体
    createSphere(vertices, indices, 1.0f, 8, 16);
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

void LavaFlow::createSphere(std::vector<float>& vertices, std::vector<unsigned int>& indices,
    float radius, int latSegments, int lonSegments) {
    vertices.clear();
    indices.clear();

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
}