#include "Terrain.h"
#include <cmath>

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

    // Base terrain noise
    float baseHeight = noise(x * 0.05f, z * 0.05f) * 5.0f;

    // Volcano shape
    float volcanoHeight = 0.0f;
    if (distance < volcanoRadius) {
        // Volcano cone
        float t = distance / volcanoRadius;
        volcanoHeight = (1.0f - t) * 30.0f;

        // Crater
        if (distance < craterRadius) {
            float craterT = distance / craterRadius;
            float craterDepth = (1.0f - craterT * craterT) * 10.0f;
            volcanoHeight -= craterDepth;
        }
    }

    return baseHeight + volcanoHeight;
}

float Terrain::noise(float x, float y) {
    // Simple pseudo-random noise
    return sin(x * 0.1f) * cos(y * 0.1f) * 0.5f + 0.5f;
}

glm::vec3 Terrain::calculateNormal(float x, float z) {
    // Calculate normal using finite differences
    float eps = 0.1f;
    float hL = generateVolcanoHeight(x - eps, z);
    float hR = generateVolcanoHeight(x + eps, z);
    float hD = generateVolcanoHeight(x, z - eps);
    float hU = generateVolcanoHeight(x, z + eps);

    glm::vec3 normal(hL - hR, 2.0f * eps, hD - hU);
    return glm::normalize(normal);
}