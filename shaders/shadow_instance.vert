#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 3) in mat4 instanceMatrix;

uniform mat4 lightSpaceMatrix;
uniform bool isLeaves;
uniform float leavesOffset;

void main()
{
    vec3 localPos = aPos;
    if (isLeaves) {
        localPos.y += leavesOffset;
    }
    
    gl_Position = lightSpaceMatrix * instanceMatrix * vec4(localPos, 1.0);
}