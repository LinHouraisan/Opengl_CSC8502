#include "LavaLake.h"
#include <cmath>
#include <iostream>

LavaLake::LavaLake(float radius, int segments) : radius(radius) {
    setupMesh(segments);
}

LavaLake::~LavaLake() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void LavaLake::setupMesh(int segments) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // 中心顶点
    vertices.push_back(0.0f);  // x
    vertices.push_back(-7.5f); // y - 稍微提高一点避免深度冲突
    vertices.push_back(0.0f);  // z
    vertices.push_back(0.0f);  // normal x
    vertices.push_back(1.0f);  // normal y
    vertices.push_back(0.0f);  // normal z
    vertices.push_back(0.5f);  // tex coord u
    vertices.push_back(0.5f);  // tex coord v

    // 圆周顶点
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * 3.14159f * i / segments;
        float x = radius * cos(angle);
        float z = radius * sin(angle);

        vertices.push_back(x);
        vertices.push_back(-7.5f); // 深度保持一致
        vertices.push_back(z);
        vertices.push_back(0.0f);  // normal x
        vertices.push_back(1.0f);  // normal y
        vertices.push_back(0.0f);  // normal z
        vertices.push_back(0.5f + 0.5f * cos(angle)); // tex coord u
        vertices.push_back(0.5f + 0.5f * sin(angle)); // tex coord v
    }

    // 生成索引
    for (int i = 0; i < segments; i++) {
        indices.push_back(0);      // 中心
        indices.push_back(i + 1);  // 当前顶点
        indices.push_back(i + 2);  // 下一个顶点
    }

    indexCount = indices.size();

    // 创建缓冲
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

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

    glBindVertexArray(0);
}

void LavaLake::Draw(Shader& shader, float time) {
    shader.setFloat("time", time);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}