#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in mat4 instanceMatrix;
layout (location = 7) in float aOpacity;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out float Opacity;

uniform mat4 view;
uniform mat4 projection;

void main() {
    // Billboard效果 - 使粒子始终面向相机
    vec4 worldPos = instanceMatrix * vec4(0.0, 0.0, 0.0, 1.0);
    vec3 cameraRight = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 cameraUp = vec3(view[0][1], view[1][1], view[2][1]);
    
    // 从实例矩阵提取缩放
    float scale = length(instanceMatrix[0]);
    
    // 计算billboard顶点位置
    vec3 vertexPos = worldPos.xyz 
        + cameraRight * aPos.x * scale
        + cameraUp * aPos.y * scale;
    
    FragPos = vertexPos;
    Normal = -vec3(view[0][2], view[1][2], view[2][2]); // 面向相机
    TexCoords = aTexCoords;
    Opacity = aOpacity;
    
    gl_Position = projection * view * vec4(vertexPos, 1.0);
}