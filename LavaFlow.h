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
    float temperature; // 1.0 = 最热，0.0 = 冷却
    float lifetime;
    bool active;
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

private:
    Terrain* terrain;
    float particleSize;
    bool isFlowing;
    float flowRate;
    float emissionTimer;

    // 粒子池
    std::vector<LavaParticle> particles;
    std::vector<glm::mat4> activeTransforms;

    // 渲染资源
    unsigned int VAO, VBO, EBO;
    unsigned int instanceVBO;
    unsigned int indexCount;

    // 发射参数
    glm::vec3 emissionCenter;
    float emissionRadius;
    int maxParticles;

    // 创建球体网格
    void setupMesh();
    void createSphere(std::vector<float>& vertices, std::vector<unsigned int>& indices,
        float radius, int latSegments, int lonSegments);

    // 发射新粒子
    void emitParticle();

    // 更新单个粒子
    void updateParticle(LavaParticle& particle, float deltaTime);

    // 更新实例缓冲区
    void updateInstanceBuffer();
};