#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform float time;

// 简单的噪声函数
float noise(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // 菲涅尔效果 - 边缘更亮
    float fresnel = 1.0 - max(dot(viewDir, norm), 0.0);
    fresnel = pow(fresnel, 2.0);
    
    // 气泡的基础颜色 - 半透明的橙红色
    vec3 bubbleColor = vec3(1.0, 0.4, 0.1);
    
    // 添加一些颜色变化
    float colorVariation = noise(TexCoords * 10.0 + time) * 0.2;
    bubbleColor += vec3(colorVariation, -colorVariation * 0.5, -colorVariation);
    
    // 气泡表面的高光
    vec3 lightDir = normalize(vec3(0.0, 1.0, 0.0));  // 假设光从上方来
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    
    // 组合颜色
    vec3 finalColor = bubbleColor * (0.6 + fresnel * 0.4) + vec3(1.0, 0.8, 0.6) * spec * 0.5;
    
    // 气泡是半透明的，带有发光效果
    float alpha = 0.4 + fresnel * 0.4;
    
    // 自发光效果
    finalColor *= 1.5;
    
    FragColor = vec4(finalColor, alpha);
}