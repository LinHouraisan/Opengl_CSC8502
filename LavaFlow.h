#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <deque>
#include "Shader.h"
#include "Terrain.h"

struct LavaParticle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 previousPosition;
    float temperature; // 1.0 = 炽热，0.0 = 冷却
    float lifetime;
    bool active;
    int emitterID;  // 标记粒子来自哪个发射器
};

// 岩浆发射器结构
struct LavaEmitter {
    glm::vec3 position;      // 发射位置
    glm::vec3 direction;     // 主要发射方向
    float intensity;         // 发射强度（影响初速度）
    float spread;           // 发射角度扩散
    float emissionRate;     // 每秒发射粒子数
    float emissionTimer;    // 发射计时器
    bool active;           // 是否激活
    float burstTimer;      // 爆发计时器
    float nextBurstTime;   // 下次爆发时间
};

class LavaFlow {
public:
    LavaFlow(Terrain* terrain, float particleSize = 0.5f);
    ~LavaFlow();

    // 开始/停止岩浆流
    void StartFlow();
    void StopFlow();
    bool IsFlowing() const { return isFlowing; }

    // 更新岩浆流
    void Update(float deltaTime);

    // 渲染岩浆流
    void Draw(Shader& shader, float time);

    // 设置岩浆湖中心（用于自动创建发射器）
    void SetLakeCenter(const glm::vec3& center);

    // 手动添加发射器
    void AddEmitter(const glm::vec3& position, const glm::vec3& direction,
        float intensity = 15.0f, float spread = 0.5f);

    // 触发火山爆发
    void TriggerEruption(float duration = 5.0f);

private:
    Terrain* terrain;
    float particleSize;
    bool isFlowing;
    bool isErupting;         // 是否正在爆发
    float eruptionTimer;     // 爆发计时器
    float eruptionDuration;  // 爆发持续时间

    // 粒子池
    std::vector<LavaParticle> particles;
    std::vector<glm::mat4> activeTransforms;

    // 发射器列表
    std::vector<LavaEmitter> emitters;
    glm::vec3 lakeCenter;

    // 渲染资源
    unsigned int VAO, VBO, EBO;
    unsigned int instanceVBO;
    unsigned int indexCount;

    // 参数
    int maxParticles;

    // 创建球体网格
    void setupMesh();
    void createSphere(std::vector<float>& vertices, std::vector<unsigned int>& indices,
        float radius, int latSegments, int lonSegments);

    // 初始化发射器
    void setupEmitters();

    // 更新发射器
    void updateEmitters(float deltaTime);

    // 从特定发射器发射粒子
    void emitParticle(const LavaEmitter& emitter);

    // 更新单个粒子
    void updateParticle(LavaParticle& particle, float deltaTime);

    // 更新实例缓冲区
    void updateInstanceBuffer();

    // 创建爆发效果
    void createBurst(const LavaEmitter& emitter, int particleCount);
};