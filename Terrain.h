#pragma once
#include "Mesh.h"
#include <memory>

class Terrain {
public:
    // Constructor
    Terrain(int gridSize = 100, float gridScale = 1.0f,
        float volcanoRadius = 40.0f, float craterRadius = 10.0f);

    // Generate terrain mesh
    std::unique_ptr<Mesh> GenerateMesh();

    // Get height at specific position
    float GetHeightAt(float x, float z);

private:
    int gridSize;
    float gridScale;
    float volcanoRadius;
    float craterRadius;

    // Generate volcano height
    float generateVolcanoHeight(float x, float z);

    // Simple noise function
    float noise(float x, float y);

    // Fractal Brownian Motion noise
    float fbmNoise(float x, float y, int octaves);

    // Ridged noise for creating sharp features
    float ridgedNoise(float x, float y);

    // Calculate normal from neighboring heights
    glm::vec3 calculateNormal(float x, float z);
};