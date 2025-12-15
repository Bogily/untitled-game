#version 330 core

out float fragColor;

in vec2 fragTexCoord;

uniform sampler2D depthTexture;
uniform sampler2D noiseTexture;

uniform mat4 projection;
uniform mat4 invProjection;

uniform vec3 samples[64];
uniform vec2 noiseScale;
uniform vec2 screenSize;

uniform int kernelSize;
uniform float radius;
uniform float bias;

const float nearPlane = 0.1;
const float farPlane = 1000.0;

// Convert depth buffer value to linear depth
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC
    return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

// Reconstruct view-space position from depth
// The depth texture from raylib is Y-flipped, so we flip when sampling
vec3 GetViewPos(vec2 screenUV)
{
    // Flip Y to sample from raylib's Y-flipped depth texture
    vec2 sampleUV = vec2(screenUV.x, 1.0 - screenUV.y);
    float depth = texture(depthTexture, sampleUV).r;
    
    // For NDC, we also need to flip Y to match the flipped texture
    // This ensures the reconstructed position is correct
    vec2 ndc = vec2(screenUV.x, 1.0 - screenUV.y) * 2.0 - 1.0;
    float ndcZ = depth * 2.0 - 1.0;
    
    // Reconstruct view position via inverse projection
    vec4 clipPos = vec4(ndc, ndcZ, 1.0);
    vec4 viewPos = invProjection * clipPos;
    viewPos /= viewPos.w;
    
    return viewPos.xyz;
}

// Get raw depth value (with Y-flip for raylib texture)
float GetDepth(vec2 screenUV)
{
    vec2 sampleUV = vec2(screenUV.x, 1.0 - screenUV.y);
    return texture(depthTexture, sampleUV).r;
}

// Estimate normal from depth buffer using cross product of derivatives
vec3 GetNormalFromDepth(vec2 screenUV)
{
    vec2 texelSize = 1.0 / screenSize;
    
    vec3 posCenter = GetViewPos(screenUV);
    vec3 posRight = GetViewPos(screenUV + vec2(texelSize.x, 0.0));
    vec3 posDown = GetViewPos(screenUV - vec2(0.0, texelSize.y));
    vec3 posLeft = GetViewPos(screenUV - vec2(texelSize.x, 0.0));
    vec3 posUp = GetViewPos(screenUV + vec2(0.0, texelSize.y));
    
    // Use central differences for better normal estimation
    vec3 dx = posRight - posLeft;
    vec3 dy = posUp - posDown;
    
    // Cross product to get normal
    vec3 normal = normalize(cross(dx, dy));
    
    // Ensure normal points towards camera (positive Z in view space)
    if (normal.z < 0.0)
        normal = -normal;
    
    return normal;
}

void main()
{
    float depth = GetDepth(fragTexCoord);
    
    // Skip background (far plane)
    if (depth >= 0.9999)
    {
        fragColor = 1.0;
        return;
    }
    
    vec3 fragPos = GetViewPos(fragTexCoord);
    vec3 normal = GetNormalFromDepth(fragTexCoord);
    
    // Sample noise - the noise texture already contains values in [-1,1] range (stored as RGB16F)
    vec3 randomVec = texture(noiseTexture, fragTexCoord * noiseScale).xyz;
    randomVec.z = 0.0; // Rotation only in tangent plane
    if (length(randomVec.xy) < 0.001)
        randomVec = vec3(1.0, 0.0, 0.0); // Fallback
    else
        randomVec = normalize(randomVec);
    
    // Create TBN matrix to transform from tangent space to view space
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    
    // Iterate through samples
    float occlusion = 0.0;
    int validSamples = 0;
    
    for(int i = 0; i < kernelSize; ++i)
    {
        // Get sample position in view space
        vec3 sampleOffset = TBN * samples[i];
        vec3 samplePos = fragPos + sampleOffset * radius;
        
        // Project sample to clip space
        vec4 clipOffset = projection * vec4(samplePos, 1.0);
        clipOffset.xyz /= clipOffset.w;
        
        // Convert from NDC [-1,1] to screen UV [0,1]
        // Note: clipOffset.y needs to be flipped because our depth texture is Y-flipped
        vec2 sampleScreenUV = vec2(clipOffset.x * 0.5 + 0.5, 1.0 - (clipOffset.y * 0.5 + 0.5));
        
        // Skip if outside screen
        if (sampleScreenUV.x < 0.0 || sampleScreenUV.x > 1.0 || 
            sampleScreenUV.y < 0.0 || sampleScreenUV.y > 1.0)
            continue;
        
        // Get the actual geometry depth at this screen position
        // Note: We use the flipped UV directly here since sampleScreenUV is already in texture space
        float geometryDepth = texture(depthTexture, sampleScreenUV).r;
        
        // Skip background samples
        if (geometryDepth >= 0.9999)
            continue;
        
        validSamples++;
        
        // Linear depth of the sample position (what we expect)
        float sampleDepthLinear = -samplePos.z;
        
        // Linear depth of actual geometry at that screen position
        float geometryDepthLinear = LinearizeDepth(geometryDepth);
        
        // Range check - only count occlusion from nearby geometry
        float rangeCheck = smoothstep(0.0, 1.0, radius / (abs(sampleDepthLinear - geometryDepthLinear) + 0.0001));
        
        // If geometry is in front of the sample position, it occludes
        // sampleDepthLinear is positive distance from camera
        // geometryDepthLinear is also positive distance
        // Occluded if geometry is closer than sample
        if (geometryDepthLinear < sampleDepthLinear - bias)
        {
            occlusion += rangeCheck;
        }
    }
    
    if (validSamples > 0)
    {
        occlusion = 1.0 - (occlusion / float(validSamples));
    }
    else
    {
        occlusion = 1.0;
    }
    
    fragColor = clamp(occlusion, 0.0, 1.0);
}
