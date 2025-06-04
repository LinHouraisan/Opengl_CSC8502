#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in float Height;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

void main() {
    // Height-based color gradient
    vec3 color;
    if (Height < 5.0) {
        // Grass - green
        color = vec3(0.2, 0.6, 0.2);
    } else if (Height < 15.0) {
        // Rock - gray
        float t = (Height - 5.0) / 10.0;
        color = mix(vec3(0.2, 0.6, 0.2), vec3(0.5, 0.5, 0.5), t);
    } else if (Height < 25.0) {
        // Volcanic rock - dark gray
        float t = (Height - 15.0) / 10.0;
        color = mix(vec3(0.5, 0.5, 0.5), vec3(0.3, 0.3, 0.3), t);
    } else {
        // Crater area - black with red tint
        color = mix(vec3(0.3, 0.3, 0.3), vec3(0.5, 0.1, 0.0), 0.5);
    }
    
    // Ambient
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;
    
    vec3 result = (ambient + diffuse + specular) * color;
    FragColor = vec4(result, 1.0);
}