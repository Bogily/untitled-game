#version 330 core

out vec4 fragColor;
in vec2 fragTexCoord;

uniform sampler2D sceneTexture;
uniform sampler2D depthTexture;
uniform mat4 invProjection;

uniform int fogEnabled;
uniform float fogDistance;
uniform float fogDensity;
uniform vec3 fogColor;

const float nearPlane = 0.1;
const float farPlane = 1000.0;

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

void main()
{
    // Sample scene (raylib texture is Y-flipped)
    vec2 sceneUV = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);
    vec3 sceneColor = texture(sceneTexture, sceneUV).rgb;
    
    if (fogEnabled == 1)
    {
        // Sample depth (raylib depth is also Y-flipped)
        float depth = texture(depthTexture, sceneUV).r;
        
        // Skip fog for background
        if (depth < 0.9999)
        {
            float linearDepth = LinearizeDepth(depth);
            
            // Exponential fog based on distance
            float fogFactor = exp(-fogDensity * max(0.0, linearDepth - fogDistance));
            fogFactor = clamp(fogFactor, 0.0, 1.0);
            
            // Mix scene color with fog color
            sceneColor = mix(fogColor, sceneColor, fogFactor);
        }
    }
    
    fragColor = vec4(sceneColor, 1.0);
}
