#include "Terrain.h"
#include <cmath>
#include <random>

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

    // 基础地形 - 平原
    float baseHeight = 0.0f;

    // 只在火山范围内生成火山地形
    if (distance < volcanoRadius * 1.5f) {
        // 火山形状
        float volcanoHeight = 0.0f;

        if (distance < volcanoRadius) {
            // 基础圆锥形状
            float t = distance / volcanoRadius;
            volcanoHeight = (1.0f - t * t) * 35.0f; // 使用平方函数让坡度更自然

            // 添加多层噪声以创建粗糙表面
            // 大尺度起伏
            float largeNoise = fbmNoise(x * 0.02f, z * 0.02f, 3) * 4.0f;

            // 中等尺度的岩石纹理
            float mediumNoise = fbmNoise(x * 0.08f, z * 0.08f, 4) * 2.0f;

            // 小尺度的粗糙度
            float smallNoise = fbmNoise(x * 0.3f, z * 0.3f, 2) * 0.8f;

            // 根据高度调整噪声强度 - 山顶附近噪声更强
            float heightFactor = volcanoHeight / 35.0f;
            volcanoHeight += largeNoise * (0.5f + heightFactor * 0.5f);
            volcanoHeight += mediumNoise * (0.3f + heightFactor * 0.7f);
            volcanoHeight += smallNoise;

            // 添加径向沟壑
            float angle = atan2(z, x);
            float groovePattern = sin(angle * 12.0f) * 0.5f + 0.5f;
            groovePattern = pow(groovePattern, 3.0f); // 让沟壑更明显
            volcanoHeight -= groovePattern * (1.0f - t) * 2.0f; // 沟壑从山顶向下逐渐变浅

            // 创建主岩浆流道
            float lavaAngle = 0.7f; // 主岩浆流道的角度（弧度）
            float angleDiff = fabs(angle - lavaAngle);
            if (angleDiff > 3.14159f) angleDiff = 2.0f * 3.14159f - angleDiff;

            if (angleDiff < 0.3f) { // 流道宽度
                float channelDepth = (1.0f - angleDiff / 0.3f) * (1.0f - t * 0.7f) * 4.0f;
                // 添加一些噪声让流道更自然
                channelDepth *= (1.0f + noise(x * 0.1f, z * 0.1f) * 0.3f);
                volcanoHeight -= channelDepth;
            }

            // 火山口
            if (distance < craterRadius) {
                float craterT = distance / craterRadius;

                // 火山口深度
                float craterDepth = (1.0f - craterT * craterT) * 12.0f;

                // 火山口边缘的随机高度变化
                float rimNoise = 0.0f;
                if (craterT > 0.7f) { // 只在边缘附近添加变化
                    float rimFactor = (craterT - 0.7f) / 0.3f;
                    // 使用角度创建不规则的边缘
                    rimNoise = sin(angle * 8.0f + noise(x * 0.5f, z * 0.5f) * 3.0f) * 2.0f;
                    rimNoise += fbmNoise(x * 0.2f, z * 0.2f, 2) * 3.0f;
                    rimNoise *= (1.0f - rimFactor); // 向内逐渐减少
                }

                volcanoHeight = volcanoHeight - craterDepth + rimNoise;

                // 确保火山口中心有一些起伏
                if (craterT < 0.3f) {
                    volcanoHeight += fbmNoise(x * 0.15f, z * 0.15f, 2) * 1.5f;
                }
            }

            // 添加一些岩石突起
            float rockNoise = ridgedNoise(x * 0.1f, z * 0.1f);
            if (rockNoise > 0.7f) {
                volcanoHeight += (rockNoise - 0.7f) * 10.0f * (1.0f - t);
            }
        }

        baseHeight = volcanoHeight;
    }

    return baseHeight;
}

float Terrain::noise(float x, float y) {
    // 改进的噪声函数
    float n = sin(x * 1.2f + y * 0.8f) * cos(y * 1.1f - x * 0.9f);
    n += sin(x * 2.4f - y * 2.1f) * cos(y * 2.3f + x * 1.9f) * 0.5f;
    n += sin(x * 4.7f + y * 3.9f) * cos(y * 4.1f - x * 3.7f) * 0.25f;
    return n * 0.5f;
}

float Terrain::fbmNoise(float x, float y, int octaves) {
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; i++) {
        value += noise(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }

    return value / maxValue;
}

float Terrain::ridgedNoise(float x, float y) {
    float n = 1.0f - fabs(noise(x, y));
    n = n * n;
    return n;
}

glm::vec3 Terrain::calculateNormal(float x, float z) {
    // Calculate normal using finite differences
    float eps = 0.5f; // 稍微增大采样距离以获得更平滑的法线
    float hL = generateVolcanoHeight(x - eps, z);
    float hR = generateVolcanoHeight(x + eps, z);
    float hD = generateVolcanoHeight(x, z - eps);
    float hU = generateVolcanoHeight(x, z + eps);

    glm::vec3 normal(hL - hR, 2.0f * eps, hD - hU);
    return glm::normalize(normal);
}