#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <random>
#include "Shader.h"

struct AshParticle {
    glm::vec3 position;
    glm::vec3 velocity;
    float size;
    float initialSize;
    float lifetime;
    float maxLifetime;
    float opacity;
    float rotation;      // 旋转角度
    float rotationSpeed; // 旋转速度
    bool active;
};

class VolcanicAsh {
public:
    VolcanicAsh(const glm::vec3& emissionCenter, float emissionRadius, int maxParticles = 1000);  // 增加默认粒子数
    ~VolcanicAsh();

    // 开始/停止火山灰喷发
    void StartEmission();
    void StopEmission();
    bool IsEmitting() const { return isEmitting; }

    // 更新粒子系统
    void Update(float deltaTime);

    // 渲染火山灰
    void Draw(Shader& shader, float time);

    // 设置喷发强度（影响初速度和生成率）
    void SetIntensity(float intensity) { emissionIntensity = intensity; }

    // 设置风向和风力
    void SetWind(const glm::vec3& windDirection, float windStrength);

private:
    glm::vec3 emissionCenter;
    float emissionRadius;
    int maxParticles;
    bool isEmitting;
    float emissionIntensity;
    float spawnTimer;
    float spawnRate;

    // 风力参数
    glm::vec3 windDirection;
    float windStrength;

    // 粒子数组
    std::vector<AshParticle> particles;
    std::vector<glm::mat4> activeTransforms;
    std::vector<float> activeOpacities;  // 每个粒子的透明度

    // 渲染资源
    unsigned int VAO, VBO, EBO;
    unsigned int instanceVBO;
    unsigned int opacityVBO;  // 用于传递透明度
    unsigned int indexCount;

    // 随机数生成器
    std::mt19937 rng;
    std::uniform_real_distribution<float> radiusDist;
    std::uniform_real_distribution<float> sizeDist;
    std::uniform_real_distribution<float> lifeDist;
    std::uniform_real_distribution<float> velocityDist;
    std::uniform_real_distribution<float> angleDist;

    // 初始化网格（使用billboard四边形）
    void setupMesh();

    // 生成新粒子
    void spawnParticle();

    // 更新单个粒子
    void updateParticle(AshParticle& particle, float deltaTime);

    // 更新实例缓冲区
    void updateInstanceBuffer();

    // Perlin噪声函数（用于模拟湍流）
    float noise(float x, float y, float z);
};