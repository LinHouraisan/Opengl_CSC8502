#include "Tree.h"
#include <iostream>

Tree::Tree() {
    setupMesh();
}

Tree::~Tree() {
    glDeleteVertexArrays(1, &trunkVAO);
    glDeleteBuffers(1, &trunkVBO);
    glDeleteBuffers(1, &trunkEBO);
    glDeleteVertexArrays(1, &leavesVAO);
    glDeleteBuffers(1, &leavesVBO);
    glDeleteBuffers(1, &leavesEBO);
    glDeleteBuffers(1, &instanceVBO);
}

void Tree::createCylinder(std::vector<float>& vertices, std::vector<unsigned int>& indices,
    float radius, float height, int segments) {
    vertices.clear();
    indices.clear();

    // 生成顶点
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * 3.14159f * i / segments;
        float x = radius * cos(angle);
        float z = radius * sin(angle);

        // 底部顶点
        vertices.push_back(x);
        vertices.push_back(0.0f);
        vertices.push_back(z);
        // 法线
        vertices.push_back(x / radius);
        vertices.push_back(0.0f);
        vertices.push_back(z / radius);

        // 顶部顶点
        vertices.push_back(x);
        vertices.push_back(height);
        vertices.push_back(z);
        // 法线
        vertices.push_back(x / radius);
        vertices.push_back(0.0f);
        vertices.push_back(z / radius);
    }

    // 生成索引
    for (int i = 0; i < segments; i++) {
        int bottom1 = i * 2;
        int top1 = bottom1 + 1;
        int bottom2 = (i + 1) * 2;
        int top2 = bottom2 + 1;

        // 第一个三角形
        indices.push_back(bottom1);
        indices.push_back(bottom2);
        indices.push_back(top1);

        // 第二个三角形
        indices.push_back(top1);
        indices.push_back(bottom2);
        indices.push_back(top2);
    }
}

void Tree::createCone(std::vector<float>& vertices, std::vector<unsigned int>& indices,
    float radius, float height, int segments) {
    vertices.clear();
    indices.clear();

    // 顶点（锥尖）
    vertices.push_back(0.0f);
    vertices.push_back(height);
    vertices.push_back(0.0f);
    // 法线（向上）
    vertices.push_back(0.0f);
    vertices.push_back(1.0f);
    vertices.push_back(0.0f);

    // 底部顶点
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * 3.14159f * i / segments;
        float x = radius * cos(angle);
        float z = radius * sin(angle);

        vertices.push_back(x);
        vertices.push_back(0.0f);
        vertices.push_back(z);

        // 计算侧面法线
        glm::vec3 normal = glm::normalize(glm::vec3(x, radius * 0.5f, z));
        vertices.push_back(normal.x);
        vertices.push_back(normal.y);
        vertices.push_back(normal.z);
    }

    // 生成侧面索引
    for (int i = 1; i <= segments; i++) {
        indices.push_back(0);  // 顶点
        indices.push_back(i);
        indices.push_back(i + 1);
    }
}

void Tree::setupMesh() {
    // 树干参数
    float trunkRadius = 0.15f;
    float trunkHeight = 2.0f;
    int segments = 8;  // 简单的8边形

    // 树冠参数
    float leavesRadius = 1.2f;
    float leavesHeight = 2.5f;

    // 创建树干网格
    std::vector<float> trunkVertices;
    std::vector<unsigned int> trunkIndices;
    createCylinder(trunkVertices, trunkIndices, trunkRadius, trunkHeight, segments);
    trunkIndexCount = trunkIndices.size();

    // 设置树干VAO
    glGenVertexArrays(1, &trunkVAO);
    glGenBuffers(1, &trunkVBO);
    glGenBuffers(1, &trunkEBO);

    glBindVertexArray(trunkVAO);

    glBindBuffer(GL_ARRAY_BUFFER, trunkVBO);
    glBufferData(GL_ARRAY_BUFFER, trunkVertices.size() * sizeof(float),
        trunkVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, trunkEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, trunkIndices.size() * sizeof(unsigned int),
        trunkIndices.data(), GL_STATIC_DRAW);

    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 法线属性
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 创建树冠网格
    std::vector<float> leavesVertices;
    std::vector<unsigned int> leavesIndices;
    createCone(leavesVertices, leavesIndices, leavesRadius, leavesHeight, segments);
    leavesIndexCount = leavesIndices.size();

    // 设置树冠VAO
    glGenVertexArrays(1, &leavesVAO);
    glGenBuffers(1, &leavesVBO);
    glGenBuffers(1, &leavesEBO);

    glBindVertexArray(leavesVAO);

    glBindBuffer(GL_ARRAY_BUFFER, leavesVBO);
    glBufferData(GL_ARRAY_BUFFER, leavesVertices.size() * sizeof(float),
        leavesVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, leavesEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, leavesIndices.size() * sizeof(unsigned int),
        leavesIndices.data(), GL_STATIC_DRAW);

    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 法线属性
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // 生成实例VBO
    glGenBuffers(1, &instanceVBO);
}

void Tree::GenerateInstances(int count, float terrainSize, float volcanoRadius) {
    instanceMatrices.clear();
    instanceMatrices.reserve(count);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDist(-terrainSize / 2, terrainSize / 2);
    std::uniform_real_distribution<float> scaleDist(0.8f, 1.2f);
    std::uniform_real_distribution<float> rotDist(0.0f, 360.0f);

    int generated = 0;
    int attempts = 0;
    const int maxAttempts = count * 10;  // 防止无限循环

    while (generated < count && attempts < maxAttempts) {
        attempts++;

        float x = posDist(gen);
        float z = posDist(gen);

        // 计算到中心的距离
        float distanceFromCenter = sqrt(x * x + z * z);

        // 只在火山范围外生成树木（留一些余量）
        if (distanceFromCenter < volcanoRadius + 5.0f) {
            continue;
        }

        // 这里简化处理，假设平原高度为0
        float y = 0.0f;

        // 创建变换矩阵
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(x, y, z));
        model = glm::rotate(model, glm::radians(rotDist(gen)), glm::vec3(0.0f, 1.0f, 0.0f));
        float scale = scaleDist(gen);
        model = glm::scale(model, glm::vec3(scale, scale, scale));

        instanceMatrices.push_back(model);
        generated++;
    }

    std::cout << "Generated " << generated << " trees out of " << count << " requested." << std::endl;

    // 更新实例缓冲区
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, instanceMatrices.size() * sizeof(glm::mat4),
        instanceMatrices.data(), GL_STATIC_DRAW);

    // 设置树干的实例属性
    glBindVertexArray(trunkVAO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

    // 实例矩阵属性（占用location 3, 4, 5, 6）
    for (int i = 0; i < 4; i++) {
        glEnableVertexAttribArray(3 + i);
        glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
            (void*)(i * sizeof(glm::vec4)));
        glVertexAttribDivisor(3 + i, 1);
    }

    // 设置树冠的实例属性
    glBindVertexArray(leavesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

    for (int i = 0; i < 4; i++) {
        glEnableVertexAttribArray(3 + i);
        glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
            (void*)(i * sizeof(glm::vec4)));
        glVertexAttribDivisor(3 + i, 1);
    }

    glBindVertexArray(0);
}

void Tree::DrawInstanced(Shader& shader) {
    if (instanceMatrices.empty()) return;

    // 绘制树干
    shader.setVec3("objectColor", glm::vec3(0.4f, 0.3f, 0.2f));  // 棕色
    glBindVertexArray(trunkVAO);
    glDrawElementsInstanced(GL_TRIANGLES, trunkIndexCount, GL_UNSIGNED_INT, 0, instanceMatrices.size());

    // 绘制树冠（需要偏移到树干顶部）
    shader.setBool("isLeaves", true);
    shader.setFloat("leavesOffset", 2.0f);  // 树干高度
    shader.setVec3("objectColor", glm::vec3(0.2f, 0.6f, 0.2f));  // 绿色
    glBindVertexArray(leavesVAO);
    glDrawElementsInstanced(GL_TRIANGLES, leavesIndexCount, GL_UNSIGNED_INT, 0, instanceMatrices.size());
    shader.setBool("isLeaves", false);

    glBindVertexArray(0);
}