#version 330

// Screen Space Ambient Occlusion (SSAO)
// Samples depth around current pixel to determine occlusion

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;       // Scene color
uniform sampler2D depthTexture;   // Normalized depth
uniform int numSamples;           // Number of samples (4-32)
uniform float radius;             // Sample radius in screen space (0.001-0.1)
uniform float bias;               // Depth bias to prevent artifacts (0.001-0.01)
uniform float intensity;          // AO intensity (0.0-2.0)
uniform float contrast;           // AO contrast (0.5-2.0)
uniform bool enabled;             // Whether to apply SSAO

out vec4 finalColor;

// Pseudo-random number generator
float random(vec2 seed)
{
    return fract(sin(dot(seed, vec2(12.9898, 78.233))) * 43758.5453);
}

// Sample depth at offset position
float sampleDepth(vec2 offset)
{
    vec2 samplePos = fragTexCoord + offset;
    if (samplePos.x < 0.0 || samplePos.x > 1.0 || samplePos.y < 0.0 || samplePos.y > 1.0)
        return texture(depthTexture, fragTexCoord).r; // Return center depth if out of bounds
    return texture(depthTexture, samplePos).r;
}

void main()
{
    vec4 color = texture(texture0, fragTexCoord);
    
    if (!enabled || intensity <= 0.0)
    {
        finalColor = color;
        return;
    }
    
    float centerDepth = texture(depthTexture, fragTexCoord).r;
    
    float occlusion = 0.0;
    float validSamples = 0.0;
    
    // Sample in a circular pattern around the current pixel
    for (int i = 0; i < numSamples; i++)
    {
        float angle = (float(i) / float(numSamples)) * 6.28318; // 2*PI
        
        // Add some randomness to reduce banding
        float randomAngle = angle + random(fragTexCoord + vec2(float(i))) * 0.5;
        
        // Sample at multiple distances
        for (int d = 1; d <= 3; d++)
        {
            float dist = radius * float(d) / 3.0;
            vec2 offset = vec2(cos(randomAngle), sin(randomAngle)) * dist;
            
            float sampleDepthValue = sampleDepth(offset);
            
            // Only count occlusion if sample is CLOSER to camera than center
            // and within reasonable distance (not at sky/far background)
            float depthDiff = sampleDepthValue - centerDepth;  // positive = sample is closer
            
            // Proper occlusion: sample is closer, past bias threshold, but not too far
            if (depthDiff > bias && depthDiff < 0.02)
            {
                occlusion += 1.0;
            }
            
            validSamples += 1.0;
        }
    }
    
    // Average the occlusion
    occlusion = occlusion / validSamples;
    
    // Apply contrast and intensity
    occlusion = pow(occlusion, contrast);
    occlusion *= intensity;
    
    // Clamp to reasonable range
    occlusion = clamp(occlusion, 0.0, 1.0);
    
    // Apply AO as darkening
    color.rgb *= (1.0 - occlusion);
    
    finalColor = color;
}
