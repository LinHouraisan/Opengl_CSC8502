#include "Skybox.h"

// 修改天空盒顶点，让底部延伸到地面以下
float skyboxVertices[] = {
    // positions          
    -1.0f,  1.0f, -1.0f,
    -1.0f, -0.2f, -1.0f,  // 底部延伸到-0.2，确保覆盖地形底部
     1.0f, -0.2f, -1.0f,
     1.0f, -0.2f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -0.2f,  1.0f,
    -1.0f, -0.2f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -0.2f,  1.0f,

     1.0f, -0.2f, -1.0f,
     1.0f, -0.2f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -0.2f, -1.0f,

    -1.0f, -0.2f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -0.2f,  1.0f,
    -1.0f, -0.2f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -0.2f, -1.0f,
    -1.0f, -0.2f,  1.0f,
     1.0f, -0.2f, -1.0f,
     1.0f, -0.2f, -1.0f,
    -1.0f, -0.2f,  1.0f,
     1.0f, -0.2f,  1.0f
};

Skybox::Skybox() {
    setupSkybox();
}

Skybox::~Skybox() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void Skybox::Draw(Shader& shader) {
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void Skybox::setupSkybox() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindVertexArray(0);
}