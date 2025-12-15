#version 330 core

out float fragColor;

in vec2 fragTexCoord;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

uniform vec3 samples[64];
uniform mat4 projection;
uniform vec2 noiseScale;

// Parameters
uniform int kernelSize;
uniform float radius;
uniform float bias;

void main()
{
    // Get input from G-buffer (view space)
    vec3 fragPos = texture(gPosition, fragTexCoord).xyz;
    vec3 normal = normalize(texture(gNormal, fragTexCoord).rgb);
    vec3 randomVec = normalize(texture(texNoise, fragTexCoord * noiseScale).xyz);
    
    // Check for empty fragment (background)
    if (length(fragPos) < 0.001)
    {
        fragColor = 1.0;
        return;
    }
    
    // Create TBN change-of-basis matrix: from tangent-space to view-space
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    
    // Iterate over the sample kernel and calculate occlusion factor
    float occlusion = 0.0;
    int validSamples = 0;
    
    for(int i = 0; i < kernelSize; ++i)
    {
        // Get sample position
        vec3 samplePos = TBN * samples[i]; // From tangent to view-space
        samplePos = fragPos + samplePos * radius; 
        
        // Project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset;    // from view to clip-space
        offset.xyz /= offset.w;          // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
        
        // Skip samples outside the screen
        if (offset.x < 0.0 || offset.x > 1.0 || offset.y < 0.0 || offset.y > 1.0)
            continue;
        
        // Get sample depth
        float sampleDepth = texture(gPosition, offset.xy).z;
        
        // Skip empty samples (background)
        if (abs(sampleDepth) < 0.001)
            continue;
        
        validSamples++;
        
        // Range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }
    
    // Avoid division by zero
    if (validSamples > 0)
    {
        occlusion = 1.0 - (occlusion / float(validSamples));
    }
    else
    {
        occlusion = 1.0;
    }
    
    fragColor = occlusion;
}
