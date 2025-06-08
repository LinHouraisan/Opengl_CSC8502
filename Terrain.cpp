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

        // 创建火山口遮罩：在火山口区域内减少或消除噪声
        float craterMask = 1.0f;
        if (distance < craterRadius * 1.2f) {  // 稍微扩大遮罩范围
            craterMask = glm::smoothstep(0.0f, 1.0f, distance / (craterRadius * 1.2f));
        }

        // 添加1条水流冲刷的沟渠（朝向正北方向）
        float grooveDepth = 0.0f;
        float grooveAngle = 0.0f;  // 正北方向
        float angleDiff = angle - grooveAngle;

        // 归一化角度差
        while (angleDiff > 3.14159f) angleDiff -= 6.28318f;
        while (angleDiff < -3.14159f) angleDiff += 6.28318f;

        if (fabs(angleDiff) < 0.15f) {
            float grooveStrength = 1.0f - fabs(angleDiff) / 0.15f;
            // 沟渠从山顶到山底，深度逐渐变浅
            float depthFactor = 1.0f - t * 0.5f;
            grooveDepth = grooveStrength * 3.0f * depthFactor * craterMask;  // 应用遮罩
        }

        // 添加4条山脊（突起，提供结构）
        float ridgeHeight = 0.0f;
        for (int i = 0; i < 4; i++) {
            float ridgeAngle = (float)i * 6.28318f / 4.0f + 0.39269f; // 偏移45度
            float angleDiff = angle - ridgeAngle;

            // 归一化角度差
            while (angleDiff > 3.14159f) angleDiff -= 6.28318f;
            while (angleDiff < -3.14159f) angleDiff += 6.28318f;

            if (fabs(angleDiff) < 0.25f) {
                // 提供支撑
                float ridgeStrength;
                if (fabs(angleDiff) < 0.1f) {
                    ridgeStrength = 1.0f; // 顶部平坦
                }
                else {
                    ridgeStrength = 1.0f - (fabs(angleDiff) - 0.1f) / 0.15f; // 斜坡
                }
                // 山脊从山顶到山底，高度逐渐降低
                float heightFactor = 1.0f - t * 0.6f;
                ridgeHeight = fmax(ridgeHeight, ridgeStrength * 4.0f * heightFactor * craterMask);  // 应用遮罩
            }
        }

        // 应用沟渠和山脊（使用遮罩减少对火山口的影响）
        baseHeight = baseHeight - grooveDepth + ridgeHeight;

        // 添加表面颗粒感（应用遮罩）
        float granularity = noise(x * 0.5f, z * 0.5f) * 0.5f * craterMask;
        granularity += noise(x * 1.0f, z * 1.0f) * 0.25f * craterMask;
        baseHeight += granularity;

        // 火山口 - 浅碗型结构
        if (distance < craterRadius) {
            float craterT = distance / craterRadius;

            // 清除之前的高度，创建干净的碗型
            baseHeight = 0.0f;

            // 创建平滑的浅碗型
            float bowlDepth = 2.5f * sqrt(1.0f - craterT * craterT);  // 使用圆形函数创建碗型
            baseHeight = 30.0f - bowlDepth;  // 从火山口边缘高度开始下降

            // 火山口边缘的小幅隆起
            if (craterT > 0.8f) {
                float rimT = (craterT - 0.8f) / 0.2f;
                float rimHeight = 1.5f * rimT * (1.0f - rimT);
                baseHeight += rimHeight;
            }

            // 在火山口边缘创建一个缺口（用于岩浆流出）
            if (fabs(angle) < 0.2f && craterT > 0.7f) {  // 正北方向的缺口
                float notchStrength = 1.0f - fabs(angle) / 0.2f;
                float notchDepth = notchStrength * 2.0f * (craterT - 0.7f) / 0.3f;
                baseHeight -= notchDepth;
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