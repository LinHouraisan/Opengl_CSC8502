#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include "Shader.h"

class LavaLake {
public:
    LavaLake(float radius = 8.0f, int segments = 32);
    ~LavaLake();

    void Draw(Shader& shader, float time);

private:
    unsigned int VAO, VBO, EBO;
    unsigned int indexCount;
    float radius;

    void setupMesh(int segments);
};