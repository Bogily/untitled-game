#version 330 core

// Input from vertex shader
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;

// Uniforms
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec3 lightDir;
uniform vec3 lightColor;

// Grass colors
uniform vec3 grassColorTop;
uniform vec3 grassColorBottom;
uniform float ambientStrength;

// Output
out vec4 finalColor;

void main()
{
    // Gradient based on height (texcoord Y)
    float heightFactor = fragTexCoord.y;
    vec3 grassColor = mix(grassColorBottom, grassColorTop, heightFactor);
    
    // Simple lighting
    vec3 normal = normalize(fragNormal);
    vec3 lightDirection = normalize(-lightDir);
    
    float diff = max(dot(normal, lightDirection), 0.0);
    vec3 diffuse = diff * lightColor;
    vec3 ambient = ambientStrength * lightColor;
    
    // Subsurface scattering approximation
    float backlight = max(0.0, dot(normal, -lightDirection));
    vec3 subsurface = vec3(0.2, 0.5, 0.1) * backlight * 0.4;
    
    vec3 lighting = ambient + diffuse + subsurface;
    vec3 result = grassColor * lighting * fragColor.rgb;
    
    // Alpha based on vertex color
    float alpha = fragColor.a;
    
    // Discard nearly transparent pixels
    if (alpha < 0.1)
        discard;
    
    finalColor = vec4(result, alpha);
}
