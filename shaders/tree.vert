#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
// 实例化矩阵属性
layout (location = 3) in mat4 instanceMatrix;

out vec3 FragPos;
out vec3 Normal;
out float Height;

uniform mat4 view;
uniform mat4 projection;
uniform bool isLeaves;
uniform float leavesOffset;

void main() {
    vec3 localPos = aPos;
    
    // 如果是树叶，需要向上偏移
    if (isLeaves) {
        localPos.y += leavesOffset;
    }
    
    // 应用实例变换
    vec4 worldPos = instanceMatrix * vec4(localPos, 1.0);
    FragPos = vec3(worldPos);
    
    // 变换法线
    Normal = mat3(transpose(inverse(instanceMatrix))) * aNormal;
    
    // 用于颜色渐变的高度
    Height = localPos.y;
    
    gl_Position = projection * view * worldPos;
}