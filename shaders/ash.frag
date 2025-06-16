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
    
    // 基础圆形渐变，但边缘更硬以创建更浓密的效果
    vec2 center = vec2(0.5);
    float dist = length(uv - center);
    float radialGradient = 1.0 - smoothstep(0.0, 0.7, dist);  // 扩大实心区域
    
    // 添加噪声扰动
    vec2 noisedUV = uv + vec2(
        sin(uv.y * 8.0 + time * 0.3) * 0.03,
        cos(uv.x * 8.0 + time * 0.2) * 0.03
    );
    
    // 创建云状纹理
    float noise1 = sin(noisedUV.x * 12.0) * cos(noisedUV.y * 12.0);
    float noise2 = sin(noisedUV.x * 25.0 + 1.57) * cos(noisedUV.y * 25.0 + 1.57);
    float noise3 = sin(noisedUV.x * 40.0) * cos(noisedUV.y * 40.0);
    float cloudiness = (noise1 + noise2 * 0.5 + noise3 * 0.25) * 0.2 + 0.8;  // 提高基础密度
    
    // 结合径向渐变和云状纹理
    pattern = radialGradient * cloudiness;
    
    // 更硬的边缘以获得更浓密的效果
    pattern = smoothstep(0.0, 0.3, pattern);
    
    return pattern;
}

void main() {
    // 生成烟雾图案
    float smoke = smokePattern(TexCoords);
    
    // 火山灰的颜色 - 更暗的颜色范围
    vec3 blackAshColor = vec3(0.05, 0.05, 0.05);  // 几乎纯黑
    vec3 darkAshColor = vec3(0.15, 0.15, 0.15);   // 深灰
    vec3 midAshColor = vec3(0.25, 0.25, 0.25);    // 中灰
    
    // 根据烟雾密度混合颜色，创建更深的效果
    vec3 ashColor;
    if (smoke > 0.7) {
        ashColor = mix(darkAshColor, blackAshColor, (smoke - 0.7) / 0.3);
    } else {
        ashColor = mix(midAshColor, darkAshColor, smoke / 0.7);
    }
    
    // 添加一些褐色调（火山灰的特征色）
    ashColor += vec3(0.02, 0.01, 0.0) * smoke;
    
    // 边缘发光效果（模拟光线散射）- 减弱效果
    vec3 viewDir = normalize(viewPos - FragPos);
    float rim = 1.0 - max(dot(viewDir, Normal), 0.0);
    rim = pow(rim, 4.0);  // 更高的幂次使边缘效果更细
    
    // 在边缘添加很轻微的亮度
    ashColor += vec3(0.05) * rim * (1.0 - smoke * 0.5);  // 中心更暗
    
    // 最终透明度 - 提高整体不透明度
    float finalAlpha = smoke * Opacity * 1.2;  // 乘以1.2使其更不透明
    finalAlpha = clamp(finalAlpha, 0.0, 1.0);
    
    // 应用预乘alpha以获得更好的混合效果
    FragColor = vec4(ashColor * finalAlpha, finalAlpha);
}