#include "Terrain.h"
#include <cmath>
#include <algorithm>

Terrain::Terrain(int gridSize, float gridScale, float volcanoRadius, float craterRadius)
    : gridSize(gridSize), gridScale(gridScale),
    volcanoRadius(volcanoRadius), craterRadius(craterRadius) {
}

std::unique_ptr<Mesh> Terrain::GenerateMesh() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // Generate vertices
    for (int z = 0; z <= gridSize; z++) {
        for (int x = 0; x <= gridSize; x++) {
            float xPos = (x - gridSize / 2) * gridScale;
            float zPos = (z - gridSize / 2) * gridScale;
            float yPos = generateVolcanoHeight(xPos, zPos);

            Vertex vertex;
            vertex.Position = glm::vec3(xPos, yPos, zPos);
            vertex.Normal = calculateNormal(xPos, zPos);
            vertex.TexCoords = glm::vec2((float)x / gridSize, (float)z / gridSize);

            vertices.push_back(vertex);
        }
    }

    // Generate indices
    for (int z = 0; z < gridSize; z++) {
        for (int x = 0; x < gridSize; x++) {
            unsigned int topLeft = z * (gridSize + 1) + x;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = (z + 1) * (gridSize + 1) + x;
            unsigned int bottomRight = bottomLeft + 1;

            // First triangle
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // Second triangle
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    return std::make_unique<Mesh>(vertices, indices);
}

float Terrain::GetHeightAt(float x, float z) {
    return generateVolcanoHeight(x, z);
}

float Terrain::generateVolcanoHeight(float x, float z) {
    float distance = sqrt(x * x + z * z);

    // 基础高度为0（平原）
    float height = 0.0f;

    // 只在火山范围内生成高度
    if (distance < volcanoRadius) {
        // 计算归一化距离
        float t = distance / volcanoRadius;

        // 更真实的火山轮廓 - 向内凹陷的曲线
        float bowlProfile;
        if (t < 0.3f) {
            // 顶部区域 - 陡峭下降
            float localT = t / 0.3f;
            // 使用立方函数创建凹陷效果
            bowlProfile = 1.0f - localT * localT * localT * 0.3f;
        }
        else if (t < 0.7f) {
            // 中部区域 - 凹陷的主体
            float localT = (t - 0.3f) / 0.4f;
            // 使用开方函数创建向内凹的曲线
            bowlProfile = 0.7f - sqrt(localT) * 0.5f;
        }
        else {
            // 底部区域 - 平缓延伸
            float localT = (t - 0.7f) / 0.3f;
            // 使用指数函数让底部更平缓
            bowlProfile = 0.2f * exp(-localT * 2.0f);
        }
        float baseHeight = bowlProfile * 35.0f;

        // 计算角度用于径向特征
        float angle = atan2(z, x);

        // 添加8条水流冲刷的沟壑（凹陷）
        float grooveDepth = 0.0f;
        for (int i = 0; i < 8; i++) {
            float grooveAngle = (float)i * 6.28318f / 8.0f;
            float angleDiff = angle - grooveAngle;

            // 归一化角度差
            while (angleDiff > 3.14159f) angleDiff -= 6.28318f;
            while (angleDiff < -3.14159f) angleDiff += 6.28318f;

            if (fabs(angleDiff) < 0.15f) {
                float grooveStrength = 1.0f - fabs(angleDiff) / 0.15f;
                // 沟壑从山顶到山底，深度逐渐变浅
                float depthFactor = 1.0f - t * 0.5f;
                grooveDepth = fmax(grooveDepth, grooveStrength * 3.0f * depthFactor);
            }
        }

        // 添加4条山脊（凸起，梯形）
        float ridgeHeight = 0.0f;
        for (int i = 0; i < 4; i++) {
            float ridgeAngle = (float)i * 6.28318f / 4.0f + 0.39269f; // 偏移45度
            float angleDiff = angle - ridgeAngle;

            // 归一化角度差
            while (angleDiff > 3.14159f) angleDiff -= 6.28318f;
            while (angleDiff < -3.14159f) angleDiff += 6.28318f;

            if (fabs(angleDiff) < 0.25f) {
                // 梯形轮廓
                float ridgeStrength;
                if (fabs(angleDiff) < 0.1f) {
                    ridgeStrength = 1.0f; // 顶部平坦
                }
                else {
                    ridgeStrength = 1.0f - (fabs(angleDiff) - 0.1f) / 0.15f; // 斜坡
                }
                // 山脊从山顶到山底，高度逐渐降低
                float heightFactor = 1.0f - t * 0.6f;
                ridgeHeight = fmax(ridgeHeight, ridgeStrength * 4.0f * heightFactor);
            }
        }

        // 应用沟壑和山脊
        baseHeight = baseHeight - grooveDepth + ridgeHeight;

        // 添加表面颗粒感
        float granularity = noise(x * 0.5f, z * 0.5f) * 0.5f;
        granularity += noise(x * 1.0f, z * 1.0f) * 0.25f;
        baseHeight += granularity;

        // 火山口 - 圆柱形环状结构
        if (distance < craterRadius) {
            float craterT = distance / craterRadius;

            // 火山口是一个圆环形的"支撑"
            if (craterT > 0.6f) {
                // 火山口环的外壁
                float rimT = (craterT - 0.6f) / 0.4f;
                float rimHeight = 5.0f * (1.0f - rimT * rimT);

                // 边缘起伏
                float rimVariation = sin(angle * 7.0f) * 1.0f;
                rimVariation += noise(x * 0.3f, z * 0.3f) * 1.5f;

                baseHeight += rimHeight + rimVariation;
            }
            else {
                // 火山口内部凹陷
                float innerDepth = 8.0f * (1.0f - craterT / 0.6f);
                baseHeight -= innerDepth;
            }
        }

        height = baseHeight;
    }

    return height;
}

float Terrain::noise(float x, float y) {
    // 简单的伪随机噪声函数
    float n = sin(x * 2.1f) * cos(y * 1.9f);
    n += sin(x * 4.3f) * cos(y * 3.7f) * 0.5f;
    return n * 0.3f;
}

glm::vec3 Terrain::calculateNormal(float x, float z) {
    // 使用有限差分计算法线
    float eps = 0.3f;
    float hL = generateVolcanoHeight(x - eps, z);
    float hR = generateVolcanoHeight(x + eps, z);
    float hD = generateVolcanoHeight(x, z - eps);
    float hU = generateVolcanoHeight(x, z + eps);

    glm::vec3 normal(hL - hR, 2.0f * eps, hD - hU);
    return glm::normalize(normal);
}