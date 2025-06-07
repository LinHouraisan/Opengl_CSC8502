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
uniform float shadowIntensity;
uniform float time;

// 岩浆光照
uniform vec3 lavaPos = vec3(0.0, -8.0, 0.0);
uniform vec3 lavaColor = vec3(1.0, 0.3, 0.0);

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if(projCoords.z > 1.0)
        return 0.0;
    
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    
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
    
    // 基础光照
    float ambientStrength = mix(0.1, 0.3, lightColor.r);
    vec3 ambient = ambientStrength * lightColor;
    
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;
    
    float shadow = ShadowCalculation(FragPosLightSpace, norm, lightDir);
    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular);
    
    // 岩浆光照
    vec3 lavaLightDir = normalize(lavaPos - FragPos);
    float lavaDistance = length(lavaPos - FragPos);
    float lavaAttenuation = 1.0 / (1.0 + 0.09 * lavaDistance + 0.032 * lavaDistance * lavaDistance);
    
    // 只有在火山口附近和面向岩浆的表面才受影响
    if (lavaDistance < 30.0 && dot(norm, lavaLightDir) > 0.0) {
        float lavaDiff = max(dot(norm, lavaLightDir), 0.0);
        vec3 lavaContribution = lavaColor * lavaDiff * lavaAttenuation;
        
        // 脉动效果
        float pulse = sin(time * 2.0) * 0.5 + 0.5;
        lavaContribution *= (0.7 + pulse * 0.3);
        
        lighting += lavaContribution * 0.5;
    }
    
    vec3 result = lighting * color;
    FragColor = vec4(result, 1.0);
}