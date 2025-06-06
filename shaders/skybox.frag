#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform float timeOfDay; // 0.0 = 中午, 0.5 = 日落/日出, 1.0 = 午夜
uniform vec3 sunDirection;

void main()
{    
    vec3 normalized = normalize(TexCoords);
    float y = normalized.y;
    
    // 计算太阳的位置和强度
    float sunDot = dot(normalized, sunDirection);
    float sunGlow = pow(max(sunDot, 0.0), 32.0);
    float sunDisc = smoothstep(0.99, 0.995, sunDot);
    
    // 根据时间定义颜色
    vec3 dayHorizon = vec3(0.7, 0.85, 1.0);
    vec3 dayTop = vec3(0.1, 0.3, 0.7);
    vec3 sunsetHorizon = vec3(1.0, 0.6, 0.3);
    vec3 sunsetTop = vec3(0.3, 0.2, 0.5);
    vec3 nightHorizon = vec3(0.05, 0.05, 0.1);
    vec3 nightTop = vec3(0.0, 0.0, 0.02);
    
    vec3 groundNear = vec3(0.3, 0.25, 0.2);
    vec3 groundFar = vec3(0.5, 0.45, 0.4);
    
    // 计算当前时段的颜色
    vec3 horizonColor, topColor;
    float sunIntensity = 1.0;
    
    if (timeOfDay < 0.25) {
        // 白天 (0.0 - 0.25)
        horizonColor = dayHorizon;
        topColor = dayTop;
        sunIntensity = 1.0;
    } else if (timeOfDay < 0.5) {
        // 日落 (0.25 - 0.5)
        float t = (timeOfDay - 0.25) * 4.0;
        horizonColor = mix(dayHorizon, sunsetHorizon, t);
        topColor = mix(dayTop, sunsetTop, t);
        sunIntensity = 1.0 - t * 0.3;
    } else if (timeOfDay < 0.75) {
        // 夜晚 (0.5 - 0.75)
        float t = (timeOfDay - 0.5) * 4.0;
        horizonColor = mix(sunsetHorizon, nightHorizon, t);
        topColor = mix(sunsetTop, nightTop, t);
        sunIntensity = 0.7 - t * 0.7;
    } else {
        // 日出 (0.75 - 1.0)
        float t = (timeOfDay - 0.75) * 4.0;
        horizonColor = mix(nightHorizon, dayHorizon, t);
        topColor = mix(nightTop, dayTop, t);
        sunIntensity = t;
    }
    
    vec3 finalColor;
    
    if (y < 0.0) {
        // 地面部分
        float groundFactor = abs(y);
        vec3 groundColor = mix(groundFar, groundNear, smoothstep(0.0, 0.2, groundFactor));
        
        // 夜晚时地面更暗
        groundColor *= mix(0.1, 1.0, sunIntensity);
        
        finalColor = mix(horizonColor, groundColor, smoothstep(0.0, 0.05, groundFactor));
    } else {
        // 天空部分
        float skyFactor = pow(y, 0.5);
        finalColor = mix(horizonColor, topColor, skyFactor);
        
        // 添加太阳
        if (sunDirection.y > -0.1) { // 太阳在地平线以上
            vec3 sunColor = vec3(1.0, 0.95, 0.8) * sunIntensity;
            
            // 日落时太阳变红
            if (timeOfDay > 0.2 && timeOfDay < 0.6) {
                sunColor = mix(sunColor, vec3(1.0, 0.4, 0.1), 
                    smoothstep(0.2, 0.4, timeOfDay) * (1.0 - smoothstep(0.4, 0.6, timeOfDay)));
            }
            
            finalColor += sunGlow * sunColor * 0.5;
            finalColor = mix(finalColor, sunColor, sunDisc);
        }
        
        // 夜晚添加星星
        if (sunIntensity < 0.5) {
            float stars = 0.0;
            vec3 p = normalized * 100.0;
            for (int i = 0; i < 3; i++) {
                p = fract(p * 1.5) - 0.5;
                float d = length(p);
                stars += smoothstep(0.05, 0.0, d) * (1.0 - sunIntensity * 2.0);
            }
            finalColor += vec3(stars) * 0.5;
        }
    }
    
    // 大气散射效果
    float atmosphereFactor = 1.0 - abs(y);
    atmosphereFactor = pow(atmosphereFactor, 2.0);
    vec3 fogColor = mix(horizonColor, vec3(0.8, 0.85, 0.9), sunIntensity);
    finalColor = mix(finalColor, fogColor, atmosphereFactor * 0.2);
    
    FragColor = vec4(finalColor, 1.0);
}