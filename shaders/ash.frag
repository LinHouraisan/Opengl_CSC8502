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
    
    // 使用3D噪声创建体积云效果
    vec3 noisePos = FragPos * 0.1 + vec3(time * 0.05);
    float density = fbm(noisePos);
    
    // 根据法线和噪声调整密度，创建云朵的不均匀感
    float edgeFactor = pow(max(dot(norm, viewDir), 0.0), 0.5);
    density *= edgeFactor;
    
    // 火山灰的颜色 - 深灰到黑色
    vec3 darkColor = vec3(0.05, 0.05, 0.05);  // 几乎黑色
    vec3 midColor = vec3(0.2, 0.2, 0.2);      // 深灰色
    vec3 lightColor = vec3(0.35, 0.35, 0.35); // 中灰色
    
    // 根据密度混合颜色
    vec3 ashColor;
    if (density > 0.6) {
        ashColor = mix(midColor, darkColor, (density - 0.6) / 0.4);
    } else {
        ashColor = mix(lightColor, midColor, density / 0.6);
    }
    
    // 添加轻微的棕色调
    ashColor += vec3(0.03, 0.02, 0.0) * density;
    
    // 边缘发光（大气散射效果）
    float rim = 1.0 - edgeFactor;
    rim = pow(rim, 3.0);
    
    // 根据高度添加一些亮度变化（高处稍亮）
    float heightFactor = (FragPos.y - 20.0) / 50.0;
    heightFactor = clamp(heightFactor, 0.0, 1.0);
    ashColor += vec3(0.1) * rim * heightFactor;
    
    // 最终透明度计算
    float finalAlpha = density * Opacity;
    
    // 确保中心部分足够不透明
    if (edgeFactor > 0.7) {
        finalAlpha = max(finalAlpha, Opacity * 0.8);
    }
    
    finalAlpha = clamp(finalAlpha, 0.0, 0.95);
    
    FragColor = vec4(ashColor, finalAlpha);
}