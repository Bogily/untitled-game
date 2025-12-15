#version 330 core

out vec4 fragColor;

in vec2 fragTexCoord;

// G-Buffer textures
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D ssao;

// Lighting parameters
uniform vec3 viewPos;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform float ambientStrength;

// SSAO settings
uniform int ssaoEnabled;
uniform float ssaoIntensity;

void main()
{
    // Retrieve data from G-Buffer
    vec3 fragPos = texture(gPosition, fragTexCoord).rgb;
    vec3 normal = normalize(texture(gNormal, fragTexCoord).rgb);
    vec3 albedo = texture(gAlbedo, fragTexCoord).rgb;
    float ao = ssaoEnabled == 1 ? texture(ssao, fragTexCoord).r : 1.0;
    
    // Apply SSAO intensity
    ao = mix(1.0, ao, ssaoIntensity);
    
    // Check if fragment is empty (no geometry)
    if (length(fragPos) < 0.001)
    {
        // Background - use a default color or discard
        fragColor = vec4(0.1, 0.1, 0.15, 1.0);
        return;
    }
    
    // Ambient lighting with SSAO
    vec3 ambient = ambientStrength * albedo * ao;
    
    // Diffuse lighting
    vec3 lightDirection = normalize(-lightDir);
    float diff = max(dot(normal, lightDirection), 0.0);
    vec3 diffuse = diff * lightColor * albedo;
    
    // Specular lighting (Blinn-Phong)
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 halfwayDir = normalize(lightDirection + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 specular = spec * lightColor * 0.3;
    
    // Combine all lighting components
    vec3 result = ambient + diffuse + specular;
    
    // Simple tone mapping
    result = result / (result + vec3(1.0));
    
    // Gamma correction
    result = pow(result, vec3(1.0 / 2.2));
    
    fragColor = vec4(result, 1.0);
}
