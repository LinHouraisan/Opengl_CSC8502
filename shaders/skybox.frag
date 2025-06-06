#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

void main()
{    
    vec3 normalized = normalize(TexCoords);
    float y = normalized.y;
    
    // 定义颜色
    vec3 skyColorHorizon = vec3(0.7, 0.85, 1.0);   // 地平线天空颜色（淡蓝）
    vec3 skyColorTop = vec3(0.1, 0.3, 0.7);        // 天顶颜色（深蓝）
    vec3 groundColorNear = vec3(0.3, 0.25, 0.2);   // 近处地面颜色（深棕）
    vec3 groundColorFar = vec3(0.5, 0.45, 0.4);    // 远处地面颜色（浅棕）
    vec3 fogColor = vec3(0.8, 0.85, 0.9);          // 雾的颜色
    
    vec3 finalColor;
    
    if (y < 0.0) {
        // 地面部分
        float groundFactor = abs(y);
        // 地面从远到近的颜色渐变
        vec3 groundColor = mix(groundColorFar, groundColorNear, smoothstep(0.0, 0.2, groundFactor));
        // 在地平线附近混合雾效果
        finalColor = mix(fogColor, groundColor, smoothstep(0.0, 0.05, groundFactor));
    } else {
        // 天空部分
        // 使用平滑插值创建更自然的天空渐变
        float skyFactor = pow(y, 0.5); // 使用幂函数让颜色变化更自然
        finalColor = mix(skyColorHorizon, skyColorTop, skyFactor);
        
        // 在地平线附近添加一点雾效果
        float fogFactor = exp(-y * 10.0);
        finalColor = mix(finalColor, fogColor, fogFactor * 0.3);
    }
    
    // 添加整体的大气散射效果
    // 根据视线与地平线的夹角调整颜色
    float atmosphereFactor = 1.0 - abs(y);
    atmosphereFactor = pow(atmosphereFactor, 2.0);
    finalColor = mix(finalColor, fogColor, atmosphereFactor * 0.2);
    
    FragColor = vec4(finalColor, 1.0);
}