#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 3) in mat4 instanceMatrix;

out vec3 FragPos;
out vec3 Normal;
out float Height;
out vec4 FragPosLightSpace;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;
uniform bool isLeaves;
uniform float leavesOffset;

void main() {
    vec3 localPos = aPos;
    
    if (isLeaves) {
        localPos.y += leavesOffset;
    }
    
    vec4 worldPos = instanceMatrix * vec4(localPos, 1.0);
    FragPos = vec3(worldPos);
    
    Normal = mat3(transpose(inverse(instanceMatrix))) * aNormal;
    Height = localPos.y;
    
    FragPosLightSpace = lightSpaceMatrix * worldPos;
    gl_Position = projection * view * worldPos;
}