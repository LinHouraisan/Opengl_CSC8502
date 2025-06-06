#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in float Height;
in vec4 FragPosLightSpace;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform sampler2D shadowMap;
uniform float shadowIntensity; // 阴影强度，夜晚时减弱

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    // 执行透视除法
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // 变换到[0,1]范围
    projCoords = projCoords * 0.5 + 0.5;
    
    // 超出光照视锥的区域没有阴影
    if(projCoords.z > 1.0)
        return 0.0;
    
    // 获取最近深度值
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    // 获取当前片段深度
    float currentDepth = projCoords.z;
    
    // 计算偏移量，防止阴影失真
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    
    // PCF软阴影
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    return shadow * shadowIntensity;
}

void main() {
    // Height-based color gradient
    vec3 color;
    if (Height < 5.0) {
        color = vec3(0.2, 0.6, 0.2);
    } else if (Height < 15.0) {
        float t = (Height - 5.0) / 10.0;
        color = mix(vec3(0.2, 0.6, 0.2), vec3(0.5, 0.5, 0.5), t);
    } else if (Height < 25.0) {
        float t = (Height - 15.0) / 10.0;
        color = mix(vec3(0.5, 0.5, 0.5), vec3(0.3, 0.3, 0.3), t);
    } else {
        color = mix(vec3(0.3, 0.3, 0.3), vec3(0.5, 0.1, 0.0), 0.5);
    }
    
    // Ambient - 根据光照强度调整
    float ambientStrength = mix(0.1, 0.3, lightColor.r); // 夜晚时环境光更暗
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;
    
    // 计算阴影
    float shadow = ShadowCalculation(FragPosLightSpace, norm, lightDir);
    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular);
    
    vec3 result = lighting * color;
    FragColor = vec4(result, 1.0);
}