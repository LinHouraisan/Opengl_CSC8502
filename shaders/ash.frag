#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in float Opacity;

uniform vec3 viewPos;
uniform float time;

// 生成3D噪声纹理
float noise3D(vec3 p) {
    return fract(sin(dot(p, vec3(12.9898, 78.233, 45.543))) * 43758.5453);
}

// 分形布朗运动
float fbm(vec3 p) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    
    for (int i = 0; i < 4; i++) {
        value += amplitude * noise3D(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    
    return value;
}

void main() {
    // 计算视角相关参数
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // 使用3D噪声创建纹理变化
    vec3 noisePos = FragPos * 0.1 + vec3(time * 0.05);
    float noiseValue = fbm(noisePos);
    
    // 火山灰的颜色 - 非常深的灰黑色
    vec3 blackColor = vec3(0.02, 0.02, 0.02);   // 几乎纯黑
    vec3 darkColor = vec3(0.08, 0.08, 0.08);    // 非常深的灰色
    vec3 midColor = vec3(0.15, 0.15, 0.15);     // 深灰色
    
    // 根据噪声值混合颜色，创建纹理变化
    vec3 ashColor;
    if (noiseValue > 0.6) {
        ashColor = mix(darkColor, blackColor, (noiseValue - 0.6) / 0.4);
    } else {
        ashColor = mix(midColor, darkColor, noiseValue / 0.6);
    }
    
    // 添加非常轻微的棕色调（火山灰特征）
    ashColor += vec3(0.02, 0.015, 0.0) * noiseValue;
    
    // 基于法线的简单光照，让球体有立体感
    float NdotL = max(dot(norm, normalize(vec3(0.0, 1.0, 0.0))), 0.0);
    ashColor *= (0.5 + 0.5 * NdotL);
    
    // 边缘稍微亮一点（大气散射）
    float rim = 1.0 - max(dot(viewDir, norm), 0.0);
    rim = pow(rim, 4.0);
    ashColor += vec3(0.05) * rim;
    
    // 完全不透明
    FragColor = vec4(ashColor, 1.0);
}