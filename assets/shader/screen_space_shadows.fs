#version 330

// Screen-Space Shadows / Contact Shadows
// Ray-marches in screen space to detect shadow contact points

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;       // Scene color
uniform sampler2D depthTexture;   // Linear depth (0-1)
uniform mat4 projection;          // Projection matrix
uniform mat4 view;                // View matrix
uniform vec3 lightDir;            // Light direction (normalized)
uniform float maxDistance;        // Maximum ray march distance (in screen units)
uniform int numSteps;             // Number of ray march steps
uniform float thickness;          // Surface thickness tolerance
uniform float intensity;          // Shadow intensity
uniform bool enabled;             // Whether to apply shadows

out vec4 finalColor;

// Linearize depth from NDC space
float LinearizeDepth(float depth, float near, float far)
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - z * (far - near));
}

void main()
{
    vec4 color = texture(texture0, fragTexCoord);
    
    if (!enabled || intensity <= 0.0)
    {
        finalColor = color;
        return;
    }
    
    // Sample depth at current pixel
    float centerDepth = texture(depthTexture, fragTexCoord).r;
    
    // Convert light direction to screen space
    // Approximate: project light direction to screen
    vec2 screenLightDir = normalize(vec2(lightDir.x, -lightDir.y)) * maxDistance;
    
    // Ray march toward light
    float shadowFactor = 1.0;
    int shadowCount = 0;
    
    for (int i = 1; i <= numSteps; i++)
    {
        float t = float(i) / float(numSteps);
        vec2 rayPos = fragTexCoord + screenLightDir * t;
        
        // Early exit if out of bounds
        if (rayPos.x < 0.0 || rayPos.x > 1.0 || rayPos.y < 0.0 || rayPos.y > 1.0)
            break;
        
        float rayDepth = texture(depthTexture, rayPos).r;
        float depthDiff = rayDepth - centerDepth;
        
        // If we found a surface above (closer to camera), it's a shadow
        if (depthDiff > thickness)
        {
            shadowFactor = 1.0 - intensity;
            shadowCount++;
            break;
        }
    }
    
    // Apply shadow
    color.rgb *= mix(1.0, shadowFactor, intensity);
    
    finalColor = color;
}
