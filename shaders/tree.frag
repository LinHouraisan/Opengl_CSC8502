#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in float Height;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform bool isLeaves;

void main() {
    vec3 color = objectColor;
    
    // 添加一些颜色变化
    if (isLeaves) {
        // 树叶从底部到顶部的颜色渐变
        float t = Height / 2.5;  // 树叶高度
        color = mix(vec3(0.15, 0.5, 0.15), vec3(0.25, 0.7, 0.25), t);
    } else {
        // 树干的颜色渐变
        float t = Height / 2.0;  // 树干高度
        color = mix(vec3(0.3, 0.2, 0.1), vec3(0.45, 0.35, 0.25), t);
    }
    
    // 环境光
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;
    
    // 漫反射
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // 镜面反射（树叶的镜面反射更弱）
    float specularStrength = isLeaves ? 0.1 : 0.3;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), isLeaves ? 16 : 32);
    vec3 specular = specularStrength * spec * lightColor;
    
    vec3 result = (ambient + diffuse + specular) * color;
    FragColor = vec4(result, 1.0);
}