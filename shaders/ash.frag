#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in float Opacity;

uniform vec3 viewPos;
uniform float time;

// 生成程序化的烟雾纹理
float smokePattern(vec2 uv) {
    // 多层噪声叠加创建烟雾效果
    float pattern = 0.0;
    
    // 基础圆形渐变
    vec2 center = vec2(0.5);
    float dist = length(uv - center);
    float radialGradient = 1.0 - smoothstep(0.0, 0.5, dist);
    
    // 添加噪声扰动
    vec2 noisedUV = uv + vec2(
        sin(uv.y * 10.0 + time * 0.5) * 0.05,
        cos(uv.x * 10.0 + time * 0.3) * 0.05
    );
    
    // 创建云状纹理
    float noise1 = sin(noisedUV.x * 15.0) * cos(noisedUV.y * 15.0);
    float noise2 = sin(noisedUV.x * 30.0 + 1.57) * cos(noisedUV.y * 30.0 + 1.57);
    float cloudiness = (noise1 + noise2) * 0.25 + 0.5;
    
    // 结合径向渐变和云状纹理
    pattern = radialGradient * cloudiness;
    
    // 软化边缘
    pattern = smoothstep(0.1, 0.9, pattern);
    
    return pattern;
}

void main() {
    // 生成烟雾图案
    float smoke = smokePattern(TexCoords);
    
    // 火山灰的颜色 - 深灰色到浅灰色的渐变
    vec3 darkAshColor = vec3(0.2, 0.2, 0.2);
    vec3 lightAshColor = vec3(0.5, 0.5, 0.5);
    
    // 根据烟雾密度混合颜色
    vec3 ashColor = mix(lightAshColor, darkAshColor, smoke);
    
    // 添加一些褐色调（火山灰通常带有褐色）
    ashColor += vec3(0.05, 0.03, 0.0) * smoke;
    
    // 边缘发光效果（模拟光线散射）
    vec3 viewDir = normalize(viewPos - FragPos);
    float rim = 1.0 - max(dot(viewDir, Normal), 0.0);
    rim = pow(rim, 3.0);
    
    // 在边缘添加轻微的亮度
    ashColor += vec3(0.1) * rim;
    
    // 最终透明度
    float finalAlpha = smoke * Opacity;
    
    // 应用预乘alpha以获得更好的混合效果
    FragColor = vec4(ashColor * finalAlpha, finalAlpha);
}