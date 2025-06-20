#include "VolcanicAsh.h"
#include <iostream>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

VolcanicAsh::VolcanicAsh(const glm::vec3& emissionCenter, float emissionRadius, int maxParticles)
    : emissionCenter(emissionCenter), emissionRadius(emissionRadius), maxParticles(maxParticles),
    isEmitting(false), emissionIntensity(1.0f), spawnTimer(0.0f), spawnRate(30.0f),
    windDirection(glm::vec3(1.0f, 0.0f, 0.0f)), windStrength(2.0f),
    rng(std::random_device{}()),
    radiusDist(0.0f, 1.0f),
    sizeDist(5.0f, 15.0f),  // 球体大小
    lifeDist(20.0f, 40.0f),  //生命周期
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
            // 在火山口周围随机位置生成
            float r = radiusDist(rng) * emissionRadius * 0.5f;
            float angle = angleDist(rng);

            particle.position = emissionCenter + glm::vec3(
                r * cos(angle),
                velocityDist(rng) * 2.0f,
                r * sin(angle)
            );

            // 初始向上速度，带有轻微的横向扰动
            particle.velocity = glm::vec3(
                velocityDist(rng) * 2.0f,
                10.0f + velocityDist(rng) * 5.0f,  // 强烈的向上速度
                velocityDist(rng) * 2.0f
            ) * emissionIntensity;

            particle.size = particle.initialSize = sizeDist(rng);
            particle.lifetime = 0.0f;
            particle.maxLifetime = lifeDist(rng);
            particle.opacity = 1.0f;  // 完全不透明
            particle.rotation = angleDist(rng);
            particle.rotationSpeed = velocityDist(rng) * 0.5f;
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
    particle.position += particle.velocity * deltaTime;

    // 应用重力（火山灰比较轻）
    particle.velocity.y -= 1.0f * deltaTime;

    // 应用浮力（热气流）
    float heightAboveEmission = particle.position.y - emissionCenter.y;
    if (heightAboveEmission < 30.0f) {
        float buoyancy = (1.0f - heightAboveEmission / 30.0f) * 4.0f;
        particle.velocity.y += buoyancy * deltaTime;
    }

    // 应用风力影响
    float heightFactor = glm::clamp(heightAboveEmission / 50.0f, 0.0f, 1.0f);
    particle.velocity += windDirection * windStrength * heightFactor * deltaTime;

    // 添加湍流效果
    float turbulence = 5.0f;
    float noiseScale = 0.1f;
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
    particle.velocity *= (1.0f - 0.5f * deltaTime);

    // 粒子扩散：随时间增大
    float expansionRate = 1.0f + lifeProgress * 2.0f;  // 最终扩大到3倍
    particle.size = particle.initialSize * expansionRate;

    // 透明度渐变 - 保持不透明
    particle.opacity = 1.0f;  // 始终保持完全不透明

    // 更新旋转
    particle.rotation += particle.rotationSpeed * deltaTime;

    // 限制粒子的活动范围
    float distanceFromCenter = glm::length(glm::vec2(particle.position.x - emissionCenter.x,
        particle.position.z - emissionCenter.z));
    if (distanceFromCenter > 100.0f || particle.position.y < emissionCenter.y - 20.0f) {
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

            // 添加一些随机旋转，让球体看起来更自然
            model = glm::rotate(model, particle.rotation, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, particle.rotation * 0.7f, glm::vec3(1.0f, 0.0f, 0.0f));

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
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // 创建低多边形球体（为了性能）
    const int latSegments = 8;
    const int lonSegments = 8;
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
    glGenBuffers(1, &opacityVBO);

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
    return (n - floor(n)) * 2.0f - 1.0f;
}