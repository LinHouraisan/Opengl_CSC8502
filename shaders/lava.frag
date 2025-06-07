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

// 分形噪声
float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    
    for (int i = 0; i < 4; i++) {
        value += amplitude * noise(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    
    return value;
}

void main() {
    // 动态纹理坐标
    vec2 uv = TexCoords;
    uv += vec2(time * 0.02, time * 0.01);
    
    // 基础岩浆颜色
    vec3 hotColor = vec3(1.0, 0.5, 0.0);    // 橙黄色
    vec3 coolColor = vec3(0.8, 0.1, 0.0);   // 深红色
    vec3 brightColor = vec3(1.0, 1.0, 0.2); // 亮黄色
    
    // 生成岩浆纹理
    float noise1 = fbm(uv * 3.0 + time * 0.1);
    float noise2 = fbm(uv * 5.0 - time * 0.15);
    float combinedNoise = (noise1 + noise2) * 0.5;
    
    // 创建岩浆"裂缝"效果
    float cracks = smoothstep(0.4, 0.6, combinedNoise);
    
    // 混合颜色
    vec3 lavaColor = mix(coolColor, hotColor, cracks);
    
    // 添加亮点
    float brightSpots = smoothstep(0.7, 0.8, combinedNoise);
    lavaColor = mix(lavaColor, brightColor, brightSpots);
    
    // 脉动效果
    float pulse = sin(time * 2.0) * 0.5 + 0.5;
    lavaColor *= (0.8 + pulse * 0.2);
    
    // 边缘发光
    vec3 viewDir = normalize(viewPos - FragPos);
    float rim = 1.0 - max(dot(viewDir, Normal), 0.0);
    rim = pow(rim, 2.0);
    lavaColor += rim * hotColor * 0.5;
    
    // 自发光
    FragColor = vec4(lavaColor, 1.0);
}