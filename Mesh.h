#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

class Mesh {
public:
    // 网格数据
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices);

    //绘制
    void Draw();

    // Destructor
    ~Mesh();

private:
    // Render data
    unsigned int VBO, EBO;

    // Initializes all the buffer objects/arrays
    void setupMesh();
};
