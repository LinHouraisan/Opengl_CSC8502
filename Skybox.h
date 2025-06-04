#pragma once
#include <GL/glew.h>
#include "Shader.h"

class Skybox {
public:
    Skybox();
    ~Skybox();

    void Draw(Shader& shader);

private:
    unsigned int VAO, VBO;
    void setupSkybox();
};
