#include "LavaFlow.h"
#include <algorithm>
#include <random>
#include <iostream>
#include <cstring>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

LavaFlow::LavaFlow(Terrain* terrain, float particleSize)
    : terrain(terrain), particleSize(particleSize), isFlowing(false),
    isErupting(false), eruptionTimer(0.0f), eruptionDuration(5.0f),
    maxParticles(1000) {  // 增加粒子数量以支持多个发射器

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

void LavaFlow::SetLakeCenter(const glm::vec3& center) {
    lakeCenter = center;
    setupEmitters();
}

void LavaFlow::setupEmitters() {
    emitters.clear();

    // 在火山口边缘创建多个发射器
    float craterRadius = 8.0f;
    int numEmitters = 6;  // 主要发射器数量

    for (int i = 0; i < numEmitters; i++) {
        float angle = (float)i * 2.0f * 3.14159f / numEmitters;
        float x = craterRadius * cos(angle);
        float z = craterRadius * sin(angle);
        float height = terrain->GetHeightAt(x, z);

        LavaEmitter emitter;
        emitter.position = glm::vec3(x, height + 1.0f, z);

        // 主要向外喷射，略微向上
        glm::vec3 outward = glm::normalize(glm::vec3(x, 0, z));
        emitter.direction = glm::normalize(outward + glm::vec3(0, 0.5f, 0));

        emitter.intensity = 10.0f + (rand() % 100) / 100.0f * 5.0f;  // 10-15的强度
        emitter.spread = 0.4f;
        emitter.emissionRate = 15.0f;  // 每秒15个粒子
        emitter.emissionTimer = 0.0f;
        emitter.active = true;
        emitter.burstTimer = 0.0f;
        emitter.nextBurstTime = 2.0f + (rand() % 100) / 100.0f * 3.0f;  // 2-5秒随机爆发

        emitters.push_back(emitter);
    }

    // 添加中心发射器（向上喷发）
    LavaEmitter centerEmitter;
    centerEmitter.position = lakeCenter;
    centerEmitter.direction = glm::vec3(0, 1, 0);  // 垂直向上
    centerEmitter.intensity = 20.0f;  // 更强的喷发
    centerEmitter.spread = 0.3f;
    centerEmitter.emissionRate = 30.0f;  // 更高的发射率
    centerEmitter.emissionTimer = 0.0f;
    centerEmitter.active = true;
    centerEmitter.burstTimer = 0.0f;
    centerEmitter.nextBurstTime = 1.0f;

    emitters.push_back(centerEmitter);

    std::cout << "Created " << emitters.size() << " lava emitters" << std::endl;
}

void LavaFlow::AddEmitter(const glm::vec3& position, const glm::vec3& direction,
    float intensity, float spread) {
    LavaEmitter emitter;
    emitter.position = position;
    emitter.direction = glm::normalize(direction);
    emitter.intensity = intensity;
    emitter.spread = spread;
    emitter.emissionRate = 10.0f;
    emitter.emissionTimer = 0.0f;
    emitter.active = true;
    emitter.burstTimer = 0.0f;
    emitter.nextBurstTime = 2.0f + (rand() % 100) / 100.0f * 3.0f;

    emitters.push_back(emitter);
}

void LavaFlow::StartFlow() {
    isFlowing = true;
    if (emitters.empty()) {
        setupEmitters();
    }
    std::cout << "Lava flow started with " << emitters.size() << " emitters!" << std::endl;
}

void LavaFlow::StopFlow() {
    isFlowing = false;
    std::cout << "Lava flow stopped!" << std::endl;
}

void LavaFlow::TriggerEruption(float duration) {
    isErupting = true;
    eruptionDuration = duration;
    eruptionTimer = 0.0f;

    // 爆发时创建大量粒子
    for (auto& emitter : emitters) {
        createBurst(emitter, 50);  // 每个发射器爆发50个粒子
    }

    std::cout << "Volcanic eruption triggered for " << duration << " seconds!" << std::endl;
}

void LavaFlow::Update(float deltaTime) {
    // 更新爆发状态
    if (isErupting) {
        eruptionTimer += deltaTime;
        if (eruptionTimer >= eruptionDuration) {
            isErupting = false;
        }
    }

    // 更新发射器
    if (isFlowing || isErupting) {
        updateEmitters(deltaTime);
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

void LavaFlow::updateEmitters(float deltaTime) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> burstDist(0.8f, 1.2f);

    for (auto& emitter : emitters) {
        if (!emitter.active && !isErupting) continue;

        // 常规发射
        emitter.emissionTimer += deltaTime;
        float rate = emitter.emissionRate;

        // 爆发时增加发射率
        if (isErupting) {
            rate *= 3.0f;
        }

        float emissionInterval = 1.0f / rate;

        while (emitter.emissionTimer >= emissionInterval) {
            emitParticle(emitter);
            emitter.emissionTimer -= emissionInterval;
        }

        // 检查是否需要爆发
        emitter.burstTimer += deltaTime;
        if (emitter.burstTimer >= emitter.nextBurstTime) {
            int burstCount = isErupting ? 30 : 15;  // 爆发粒子数
            createBurst(emitter, burstCount);

            emitter.burstTimer = 0.0f;
            emitter.nextBurstTime = 2.0f + burstDist(gen) * 3.0f;
        }
    }
}

void LavaFlow::emitParticle(const LavaEmitter& emitter) {
    // 找到一个非活跃粒子
    for (auto& particle : particles) {
        if (!particle.active) {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_real_distribution<float> spreadDist(-1.0f, 1.0f);
            static std::uniform_real_distribution<float> speedDist(0.8f, 1.2f);
            static std::uniform_real_distribution<float> offsetDist(-0.5f, 0.5f);

            // 在发射点附近随机位置
            particle.position = emitter.position + glm::vec3(
                offsetDist(gen),
                offsetDist(gen) * 0.5f,
                offsetDist(gen)
            );

            particle.previousPosition = particle.position;

            // 计算发射方向（带扩散）
            glm::vec3 spread = glm::vec3(
                spreadDist(gen) * emitter.spread,
                spreadDist(gen) * emitter.spread * 0.5f,  // 垂直扩散较小
                spreadDist(gen) * emitter.spread
            );

            glm::vec3 direction = glm::normalize(emitter.direction + spread);
            float speed = emitter.intensity * speedDist(gen);

            // 爆发时增加速度
            if (isErupting) {
                speed *= 1.5f;
            }

            particle.velocity = direction * speed;
            particle.temperature = 1.0f;
            particle.lifetime = 0.0f;
            particle.active = true;
            particle.emitterID = &emitter - &emitters[0];  // 记录发射器ID

            break;
        }
    }
}

void LavaFlow::createBurst(const LavaEmitter& emitter, int particleCount) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);
    static std::uniform_real_distribution<float> speedDist(0.7f, 1.3f);
    static std::uniform_real_distribution<float> upDist(0.3f, 0.7f);

    int created = 0;
    for (auto& particle : particles) {
        if (!particle.active && created < particleCount) {
            // 球形爆发模式
            float theta = angleDist(gen);
            float phi = angleDist(gen) * 0.5f;  // 限制在上半球

            glm::vec3 burstDir = glm::vec3(
                sin(phi) * cos(theta),
                cos(phi) * upDist(gen),  // 偏向向上
                sin(phi) * sin(theta)
            );

            // 结合发射器方向
            burstDir = glm::normalize(burstDir * 0.5f + emitter.direction);

            particle.position = emitter.position;
            particle.previousPosition = particle.position;
            particle.velocity = burstDir * emitter.intensity * speedDist(gen) * 1.5f;
            particle.temperature = 1.0f;
            particle.lifetime = 0.0f;
            particle.active = true;
            particle.emitterID = &emitter - &emitters[0];

            created++;
        }
    }
}

void LavaFlow::updateParticle(LavaParticle& particle, float deltaTime) {
    // 保存前一帧位置
    particle.previousPosition = particle.position;

    // 更新生命周期
    particle.lifetime += deltaTime;

    // 温度随时间降低
    particle.temperature = std::max(0.0f, 1.0f - particle.lifetime * 0.015f);

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
        if (speed > 20.0f) {
            particle.velocity = (particle.velocity / speed) * 20.0f;
        }
    }

    particle.position = newPos;

    // 停止条件
    float speed = glm::length(particle.velocity);
    if (particle.temperature <= 0.05f ||
        speed < 0.05f ||
        (terrainHeight < 1.0f && speed < 0.2f) ||
        particle.lifetime > 120.0f ||
        particle.position.y < -10.0f) {  // 防止粒子掉落太深
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

                // 计算速度的大小
                float speed = glm::length(particle.velocity);

                // 计算拉伸因子（速度越快，拉伸越长）
                float stretchFactor = 1.0f + speed * 0.4f;
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

                // 应用拉伸：Y轴（运动方向）拉伸，XZ轴略微压缩
                float baseScale = particleSize * (0.5f + 0.5f * particle.temperature);
                float compressFactor = 1.0f / sqrt(stretchFactor);
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
    shader.setBool("isErupting", isErupting);
    shader.setFloat("eruptionIntensity", isErupting ? 1.0f : 0.0f);

    glBindVertexArray(VAO);
    glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0, activeTransforms.size());
    glBindVertexArray(0);
}

void LavaFlow::setupMesh() {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // 创建球体，增加纵向分段以获得更好的拉伸效果
    createSphere(vertices, indices, 1.0f, 16, 16);
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