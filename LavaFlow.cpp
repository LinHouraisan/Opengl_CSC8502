#include "LavaFlow.h"
#include <algorithm>
#include <random>
#include <iostream>
#include <cstring>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

LavaFlow::LavaFlow(Terrain* terrain, float particleSize)
    : terrain(terrain), particleSize(particleSize), isFlowing(false),
    flowRate(8.0f), emissionTimer(0.0f), maxParticles(200) {  // 适中的发射频率

    // 初始发射中心在火山口边缘（朝北方向）
    float craterRadius = 8.0f;
    float angle = 0.0f; // 朝北方向
    float x = craterRadius * cos(angle);
    float z = craterRadius * sin(angle);
    float height = terrain->GetHeightAt(x, z);

    emissionCenter = glm::vec3(x, height + 1.0f, z);
    emissionRadius = 3.0f;

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
            static std::uniform_real_distribution<float> angleDist(-0.3f, 0.3f);
            static std::uniform_real_distribution<float> radiusDist(0.0f, 1.0f);
            static std::uniform_real_distribution<float> speedDist(3.0f, 6.0f);

            // 在发射点附近随机位置生成
            float angleOffset = angleDist(gen);
            float r = radiusDist(gen) * emissionRadius;

            particle.position = emissionCenter + glm::vec3(
                r * cos(angleOffset),
                radiusDist(gen) * 0.5f,
                r * sin(angleOffset)
            );

            particle.previousPosition = particle.position;

            // 初始速度：主要向火山外侧和向下
            float speed = speedDist(gen);
            glm::vec3 awayFromCenter = glm::normalize(particle.position - glm::vec3(0.0f, particle.position.y, 0.0f));

            particle.velocity = glm::vec3(
                awayFromCenter.x * speed,
                -2.0f - radiusDist(gen),
                awayFromCenter.z * speed
            );

            particle.temperature = 1.0f;
            particle.lifetime = 0.0f;
            particle.active = true;

            break;
        }
    }
}

void LavaFlow::updateParticle(LavaParticle& particle, float deltaTime) {
    // 保存前一帧位置
    particle.previousPosition = particle.position;

    // 更新生命周期
    particle.lifetime += deltaTime;

    // 温度随时间降低
    particle.temperature = std::max(0.0f, 1.0f - particle.lifetime * 0.02f);

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

        // 添加沿斜坡向下的力
        glm::vec3 downSlope = glm::vec3(0.0f, -1.0f, 0.0f) - normal * glm::dot(glm::vec3(0.0f, -1.0f, 0.0f), normal);
        downSlope = glm::normalize(downSlope);
        particle.velocity += downSlope * 8.0f * deltaTime;

        // 应用摩擦力
        particle.velocity *= 0.92f;

        // 限制最大速度
        float speed = glm::length(particle.velocity);
        if (speed > 15.0f) {
            particle.velocity = (particle.velocity / speed) * 15.0f;
        }
    }

    particle.position = newPos;

    // 停止条件
    float speed = glm::length(particle.velocity);
    if (particle.temperature <= 0.05f ||
        speed < 0.05f ||
        (terrainHeight < 1.0f && speed < 0.2f) ||
        particle.lifetime > 120.0f) {
        particle.active = false;
    }
}

void LavaFlow::updateInstanceBuffer() {
    activeTransforms.clear();

    for (const auto& particle : particles) {
        if (particle.active) {
            // 计算运动方向
            glm::vec3 direction = particle.position - particle.previousPosition;
            float distance = glm::length(direction);

            // 创建基础变换矩阵
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, particle.position);

            // 如果粒子在运动，根据速度拉伸
            if (distance > 0.001f) {
                direction = glm::normalize(direction);

                // 计算速度的大小（使用实际速度而不是帧间距离）
                float speed = glm::length(particle.velocity);

                // 计算拉伸因子（速度越快，拉伸越长）
                float stretchFactor = 1.0f + speed * 0.2f;  // 最多拉伸到原来的4倍
                stretchFactor = std::min(stretchFactor, 4.0f);

                // 使用速度方向而不是位移方向
                glm::vec3 velocityDir = glm::normalize(particle.velocity);

                // 计算旋转四元数，将球体的Y轴对齐到运动方向
                glm::vec3 up(0.0f, 1.0f, 0.0f);
                float dot = glm::dot(up, velocityDir);

                if (std::abs(dot - 1.0f) < 0.0001f) {
                    // 已经对齐，不需要旋转
                }
                else if (std::abs(dot + 1.0f) < 0.0001f) {
                    // 完全相反，旋转180度
                    glm::vec3 axis(1.0f, 0.0f, 0.0f);
                    model = glm::rotate(model, glm::radians(180.0f), axis);
                }
                else {
                    // 一般情况，计算旋转轴和角度
                    glm::vec3 axis = glm::normalize(glm::cross(up, velocityDir));
                    float angle = std::acos(dot);
                    model = glm::rotate(model, angle, axis);
                }

                // 应用拉伸：Y轴（运动方向）拉伸，XZ轴稍微压缩
                float baseScale = particleSize * (0.5f + 0.5f * particle.temperature);
                float compressFactor = 1.0f / sqrt(stretchFactor);  // 保持体积大致不变
                model = glm::scale(model, glm::vec3(
                    baseScale * compressFactor,
                    baseScale * stretchFactor,
                    baseScale * compressFactor
                ));
            }
            else {
                // 静止或移动很慢的粒子保持球形
                float scale = particleSize * (0.5f + 0.5f * particle.temperature);
                model = glm::scale(model, glm::vec3(scale));
            }

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

    // 创建球体，增加纵向分段以获得更好的拉伸效果
    createSphere(vertices, indices, 1.0f, 16, 16);  // 增加分段数
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