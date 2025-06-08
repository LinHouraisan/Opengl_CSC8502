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
    // 基于高度估算温度（高处更热）
    float heightFactor = clamp((FragPos.y - 0.0) / 30.0, 0.0, 1.0);
    float temperature = 0.3 + heightFactor * 0.7;
    
    // 根据温度决定颜色
    vec3 hotColor = vec3(1.0, 0.9, 0.3);    // 炽热的黄色
    vec3 warmColor = vec3(1.0, 0.4, 0.0);   // 橙红色
    vec3 coolColor = vec3(0.5, 0.0, 0.0);   // 暗红色
    vec3 coldColor = vec3(0.1, 0.0, 0.0);   // 几乎黑色
    
    vec3 lavaColor;
    if (temperature > 0.7) {
        float t = (temperature - 0.7) / 0.3;
        lavaColor = mix(warmColor, hotColor, t);
    } else if (temperature > 0.4) {
        float t = (temperature - 0.4) / 0.3;
        lavaColor = mix(coolColor, warmColor, t);
    } else {
        float t = temperature / 0.4;
        lavaColor = mix(coldColor, coolColor, t);
    }
    
    // 添加表面纹理变化
    vec2 uv = TexCoords + vec2(time * 0.05);
    float n = noise(uv * 10.0);
    lavaColor *= (0.8 + n * 0.2);
    
    // 边缘发光效果
    vec3 viewDir = normalize(viewPos - FragPos);
    float rim = 1.0 - max(dot(viewDir, normalize(Normal)), 0.0);
    rim = pow(rim, 2.0);
    
    // 根据温度调整发光强度
    vec3 glowColor = mix(coolColor, hotColor, temperature);
    lavaColor += rim * glowColor * temperature * 0.5;
    
    // 自发光 - 温度越高越亮
    float emission = temperature * temperature;
    
    FragColor = vec4(lavaColor * (1.0 + emission), 1.0);
}