#include "VolcanicAsh.h"
#include <iostream>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

VolcanicAsh::VolcanicAsh(const glm::vec3& emissionCenter, float emissionRadius, int maxParticles)
    : emissionCenter(emissionCenter), emissionRadius(emissionRadius), maxParticles(maxParticles),
    isEmitting(false), emissionIntensity(1.0f), spawnTimer(0.0f), spawnRate(50.0f),  // 增加生成率
    windDirection(glm::vec3(1.0f, 0.0f, 0.0f)), windStrength(2.0f),
    rng(std::random_device{}()),
    radiusDist(0.0f, 1.0f),
    sizeDist(8.0f, 20.0f),  // 大幅增加粒子大小
    lifeDist(30.0f, 60.0f),  // 延长生命周期
    velocityDist(-1.0f, 1.0f),
    angleDist(0.0f, 6.28318f) {

    particles.resize(maxParticles);
    for (auto& particle : particles) {
        particle.active = false;
    }

    setupMesh();
}

VolcanicAsh::~VolcanicAsh() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &instanceVBO);
    glDeleteBuffers(1, &opacityVBO);
}

void VolcanicAsh::StartEmission() {
    isEmitting = true;
    std::cout << "Volcanic ash emission started!" << std::endl;
}

void VolcanicAsh::StopEmission() {
    isEmitting = false;
    std::cout << "Volcanic ash emission stopped!" << std::endl;
}

void VolcanicAsh::SetWind(const glm::vec3& windDir, float strength) {
    windDirection = glm::normalize(windDir);
    windStrength = strength;
}

void VolcanicAsh::Update(float deltaTime) {
    // 生成新粒子
    if (isEmitting) {
        spawnTimer += deltaTime;
        float spawnInterval = 1.0f / (spawnRate * emissionIntensity);

        while (spawnTimer >= spawnInterval) {
            spawnParticle();
            spawnTimer -= spawnInterval;
        }
    }

    // 更新现有粒子
    for (auto& particle : particles) {
        if (particle.active) {
            updateParticle(particle, deltaTime);
        }
    }

    // 更新实例缓冲区
    updateInstanceBuffer();
}

void VolcanicAsh::spawnParticle() {
    for (auto& particle : particles) {
        if (!particle.active) {
            // 在火山口周围随机位置生成，但集中在中心
            float r = radiusDist(rng) * emissionRadius * 0.5f;  // 缩小发射范围使其更集中
            float angle = angleDist(rng);

            particle.position = emissionCenter + glm::vec3(
                r * cos(angle),
                velocityDist(rng) * 2.0f,  // 在发射点上下有些随机偏移
                r * sin(angle)
            );

            // 初始向上速度，带有轻微的横向扰动
            particle.velocity = glm::vec3(
                velocityDist(rng) * 1.0f,
                12.0f + velocityDist(rng) * 6.0f,  // 更强的向上速度
                velocityDist(rng) * 1.0f
            ) * emissionIntensity;

            particle.size = particle.initialSize = sizeDist(rng);
            particle.lifetime = 0.0f;
            particle.maxLifetime = lifeDist(rng);
            particle.opacity = 0.9f;  // 提高初始不透明度
            particle.rotation = angleDist(rng);
            particle.rotationSpeed = velocityDist(rng) * 1.0f;  // 减慢旋转
            particle.active = true;

            break;
        }
    }
}

void VolcanicAsh::updateParticle(AshParticle& particle, float deltaTime) {
    particle.lifetime += deltaTime;

    // 检查是否超过生命周期
    if (particle.lifetime >= particle.maxLifetime) {
        particle.active = false;
        return;
    }

    // 生命周期进度
    float lifeProgress = particle.lifetime / particle.maxLifetime;

    // 更新位置
    glm::vec3 oldPos = particle.position;
    particle.position += particle.velocity * deltaTime;

    // 应用重力（火山灰比较轻，重力影响较小）
    particle.velocity.y -= 0.5f * deltaTime;  // 很小的重力

    // 应用浮力（热气流会让火山灰上升）
    float heightAboveEmission = particle.position.y - emissionCenter.y;
    if (heightAboveEmission < 50.0f) {  // 在一定高度内有上升气流
        float buoyancy = (1.0f - heightAboveEmission / 50.0f) * 3.0f;
        particle.velocity.y += buoyancy * deltaTime;
    }

    // 应用风力影响（随着高度增加，风力影响增大）
    float heightFactor = glm::clamp(heightAboveEmission / 100.0f, 0.0f, 1.0f);
    particle.velocity += windDirection * windStrength * heightFactor * deltaTime * 0.5f;

    // 添加湍流效果
    float turbulence = 3.0f;
    float noiseScale = 0.05f;
    particle.velocity.x += turbulence * noise(
        particle.position.x * noiseScale,
        particle.position.y * noiseScale,
        particle.position.z * noiseScale
    ) * deltaTime;
    particle.velocity.z += turbulence * noise(
        particle.position.x * noiseScale + 100.0f,
        particle.position.y * noiseScale,
        particle.position.z * noiseScale
    ) * deltaTime;

    // 空气阻力
    particle.velocity *= (1.0f - 0.3f * deltaTime);

    // 粒子扩散：随时间增大，但速度较慢
    float expansionRate = 1.0f + lifeProgress * 1.5f;  // 最终扩大到2.5倍
    particle.size = particle.initialSize * expansionRate;

    // 透明度渐变：保持较长时间的不透明
    if (lifeProgress < 0.1f) {
        // 初始淡入
        particle.opacity = 0.9f * (lifeProgress / 0.1f);
    }
    else if (lifeProgress > 0.8f) {
        // 最后淡出
        particle.opacity = 0.9f * (1.0f - (lifeProgress - 0.8f) / 0.2f);
    }
    else {
        // 中间阶段保持高不透明度
        particle.opacity = 0.9f - (lifeProgress - 0.1f) * 0.3f;  // 缓慢降低透明度
    }

    // 更新旋转
    particle.rotation += particle.rotationSpeed * deltaTime;

    // 限制粒子的活动范围
    float distanceFromCenter = glm::length(glm::vec2(particle.position.x - emissionCenter.x,
        particle.position.z - emissionCenter.z));
    if (distanceFromCenter > 150.0f || particle.position.y < emissionCenter.y - 20.0f) {
        particle.active = false;
    }
}

void VolcanicAsh::updateInstanceBuffer() {
    activeTransforms.clear();
    activeOpacities.clear();

    for (const auto& particle : particles) {
        if (particle.active) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, particle.position);

            // 旋转使billboard面向相机（简化处理，实际应该在shader中做）
            model = glm::rotate(model, particle.rotation, glm::vec3(0.0f, 1.0f, 0.0f));

            // 缩放
            model = glm::scale(model, glm::vec3(particle.size));

            activeTransforms.push_back(model);
            activeOpacities.push_back(particle.opacity);
        }
    }

    // 更新GPU缓冲区
    if (!activeTransforms.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, activeTransforms.size() * sizeof(glm::mat4),
            activeTransforms.data(), GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, opacityVBO);
        glBufferData(GL_ARRAY_BUFFER, activeOpacities.size() * sizeof(float),
            activeOpacities.data(), GL_DYNAMIC_DRAW);
    }
}

void VolcanicAsh::Draw(Shader& shader, float time) {
    if (activeTransforms.empty()) return;

    shader.setFloat("time", time);

    glBindVertexArray(VAO);
    glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0,
        activeTransforms.size());
    glBindVertexArray(0);
}

void VolcanicAsh::setupMesh() {
    // 创建billboard四边形
    float vertices[] = {
        // 位置           // 法线         // 纹理坐标
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
         1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f
    };

    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    indexCount = 6;

    // 创建VAO/VBO/EBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glGenBuffers(1, &instanceVBO);
    glGenBuffers(1, &opacityVBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

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

    // 透明度属性
    glBindBuffer(GL_ARRAY_BUFFER, opacityVBO);
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);
    glVertexAttribDivisor(7, 1);

    glBindVertexArray(0);
}

float VolcanicAsh::noise(float x, float y, float z) {
    // 简单的伪随机噪声函数
    float n = sin(x * 12.9898f + y * 78.233f + z * 37.719f) * 43758.5453f;
    return (n - floor(n)) * 2.0f - 1.0f;  // fract(n) = n - floor(n)
}