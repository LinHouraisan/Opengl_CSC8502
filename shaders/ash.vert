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
uniform float time;

void main() {
    // 从实例矩阵计算世界坐标
    vec4 worldPos = instanceMatrix * vec4(aPos, 1.0);
    
    // 添加一些顶点动画，让云朵看起来更生动
    vec3 animatedPos = aPos;
    float wave = sin(worldPos.x * 0.1 + time * 0.5) * cos(worldPos.z * 0.1 + time * 0.3);
    animatedPos += aNormal * wave * 0.1;
    
    // 重新计算世界位置
    worldPos = instanceMatrix * vec4(animatedPos, 1.0);
    
    FragPos = vec3(worldPos);
    Normal = mat3(transpose(inverse(instanceMatrix))) * aNormal;
    TexCoords = aTexCoords;
    Opacity = aOpacity;
    
    gl_Position = projection * view * worldPos;
}