#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in mat4 instanceMatrix;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 view;
uniform mat4 projection;
uniform float time;

void main() {
    // 添加一些顶点扭曲，让气泡看起来更生动
    vec3 pos = aPos;
    float distortion = sin(time * 3.0 + aPos.y * 5.0) * 0.05;
    pos.x += distortion * aNormal.x;
    pos.z += distortion * aNormal.z;
    
    vec4 worldPos = instanceMatrix * vec4(pos, 1.0);
    FragPos = vec3(worldPos);
    Normal = mat3(transpose(inverse(instanceMatrix))) * aNormal;
    TexCoords = aTexCoords;
    
    gl_Position = projection * view * worldPos;
}