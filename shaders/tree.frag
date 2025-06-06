#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in float Height;
in vec4 FragPosLightSpace;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform bool isLeaves;
uniform sampler2D shadowMap;
uniform float shadowIntensity;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if(projCoords.z > 1.0)
        return 0.0;
    
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    
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
    vec3 color = objectColor;
    
    if (isLeaves) {
        float t = Height / 2.5;
        color = mix(vec3(0.15, 0.5, 0.15), vec3(0.25, 0.7, 0.25), t);
    } else {
        float t = Height / 2.0;
        color = mix(vec3(0.3, 0.2, 0.1), vec3(0.45, 0.35, 0.25), t);
    }
    
    // 环境光 - 根据光照强度调整
    float ambientStrength = mix(0.1, 0.3, lightColor.r);
    vec3 ambient = ambientStrength * lightColor;
    
    // 漫反射
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // 镜面反射
    float specularStrength = isLeaves ? 0.1 : 0.3;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), isLeaves ? 16 : 32);
    vec3 specular = specularStrength * spec * lightColor;
    
    // 计算阴影
    float shadow = ShadowCalculation(FragPosLightSpace, norm, lightDir);
    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular);
    
    vec3 result = lighting * color;
    FragColor = vec4(result, 1.0);
}