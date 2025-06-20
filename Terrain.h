#pragma once
#include "Mesh.h"
#include <memory>

class Terrain {
public:

    Terrain(int gridSize = 100, float gridScale = 1.0f, float volcanoRadius = 40.0f, float craterRadius = 10.0f);
    std::unique_ptr<Mesh> GenerateMesh();
    float GetHeightAt(float x, float z);
    glm::vec3 calculateNormal(float x, float z);

private:
    int gridSize;
    float gridScale;
    float volcanoRadius;
    float craterRadius;
    float generateVolcanoHeight(float x, float z);
    float noise(float x, float y);
};