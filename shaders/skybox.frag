#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

void main()
{    
    float y = normalize(TexCoords).y;
    vec3 skyColorBottom = vec3(0.7, 0.8, 0.9);  // Ç³À¶É«
    vec3 skyColorTop = vec3(0.2, 0.4, 0.8);     // ÉîÀ¶É«
    vec3 skyColor = mix(skyColorBottom, skyColorTop, y * 0.5 + 0.5);
    FragColor = vec4(skyColor, 1.0);
}