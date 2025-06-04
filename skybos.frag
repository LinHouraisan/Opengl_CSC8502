#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

void main() {
    // Simple gradient sky
    float y = normalize(TexCoords).y;
    vec3 skyColor = mix(vec3(0.7, 0.8, 0.9), vec3(0.2, 0.4, 0.8), y * 0.5 + 0.5);
    FragColor = vec4(skyColor, 1.0);
}