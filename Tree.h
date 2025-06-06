#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <random>
#include "Shader.h"

class Terrain;
class Tree {
public:
    Tree();
    ~Tree();

    // 生成树木实例位置 - 添加 Terrain 参数
    void GenerateInstances(int count, float terrainSize, float volcanoRadius, Terrain* terrain);

    // 绘制所有树木实例
    void DrawInstanced(Shader& shader);

private:
    // 树干和树冠的VAO/VBO
    unsigned int trunkVAO, trunkVBO, trunkEBO;
    unsigned int leavesVAO, leavesVBO, leavesEBO;
    unsigned int instanceVBO;

    // 索引数量
    unsigned int trunkIndexCount;
    unsigned int leavesIndexCount;

    // 实例数据
    std::vector<glm::mat4> instanceMatrices;

    // 创建圆柱体（树干）
    void createCylinder(std::vector<float>& vertices, std::vector<unsigned int>& indices,
        float radius, float height, int segments);

    // 创建圆锥体（树冠）
    void createCone(std::vector<float>& vertices, std::vector<unsigned int>& indices,
        float radius, float height, int segments);

    // 设置网格
    void setupMesh();
};