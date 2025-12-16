#version 330 core

out float fragColor;
in vec2 fragTexCoord;

uniform sampler2D depthTexture;
uniform sampler2D noiseTexture;
uniform mat4 projection;
uniform mat4 invProjection;
uniform vec3 samples[1];
uniform vec2 noiseScale;
uniform vec2 screenSize;
uniform int kernelSize;
uniform float radius;
uniform float bias;

// Raylib render textures are Y-flipped in OpenGL sampling.
// Treat incoming fragTexCoord as top-left UV and flip Y when sampling the depth texture.
float SampleDepth(vec2 uvTopLeft)
{
    return texture(depthTexture, vec2(uvTopLeft.x, 1.0 - uvTopLeft.y)).r;
}

vec3 reconstructViewPos(vec2 uvTopLeft, float depth)
{
    // Convert UV (0,0 top-left) to NDC (Y-up)
    vec2 ndc = vec2(uvTopLeft.x * 2.0 - 1.0, (1.0 - uvTopLeft.y) * 2.0 - 1.0);

    vec4 clipSpace = vec4(ndc, depth * 2.0 - 1.0, 1.0);
    vec4 viewSpace = invProjection * clipSpace;
    return viewSpace.xyz / viewSpace.w;
}

vec3 computeNormal(vec2 uvTopLeft)
{
    vec2 texelSize = 1.0 / screenSize;

    float depth = SampleDepth(uvTopLeft);
    if (depth > 0.999) return vec3(0.0, 0.0, 1.0);

    float depthRight = SampleDepth(uvTopLeft + vec2(texelSize.x, 0.0));
    float depthUp = SampleDepth(uvTopLeft - vec2(0.0, texelSize.y)); // up in screen space

    vec3 p = reconstructViewPos(uvTopLeft, depth);
    vec3 pRight = reconstructViewPos(uvTopLeft + vec2(texelSize.x, 0.0), depthRight);
    vec3 pUp = reconstructViewPos(uvTopLeft - vec2(0.0, texelSize.y), depthUp);

    vec3 v1 = pRight - p;
    vec3 v2 = pUp - p;

    if (dot(v1, v1) < 0.00001 || dot(v2, v2) < 0.00001)
        return vec3(0.0, 0.0, 1.0);

    vec3 normal = cross(v1, v2);
    float len = length(normal);

    if (len < 0.00001)
        return vec3(0.0, 0.0, 1.0);

    return normalize(normal);
}

void main()
{
    float depth = SampleDepth(fragTexCoord);
    
    if (depth > 0.999)
    {
        fragColor = 1.0;
        return;
    }
    
    vec3 viewPos = reconstructViewPos(fragTexCoord, depth);
    vec3 normal = computeNormal(fragTexCoord);
    vec3 randomVec = normalize(texture(noiseTexture, fragTexCoord * noiseScale).xyz);
    
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    // Safety check for tangent
    if (length(tangent) < 0.0001) tangent = vec3(1.0, 0.0, 0.0);
    
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    
    float occlusion = 0.0;
    
    for (int i = 0; i < kernelSize; i++)
    {
        vec3 samplePos = viewPos + TBN * samples[i] * radius;
        
        vec4 offset = projection * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        
        // Convert NDC to UV (0,0 top-left)
        // NDC (-1, 1) -> UV (0, 0)
        // NDC (1, -1) -> UV (1, 1)
        vec2 offsetUV = vec2(offset.x * 0.5 + 0.5, 0.5 - offset.y * 0.5);
        
        if (offsetUV.x < 0.0 || offsetUV.x > 1.0 || offsetUV.y < 0.0 || offsetUV.y > 1.0)
            continue;
        
        float sampleDepth = SampleDepth(offsetUV);
        if (sampleDepth > 0.999)
            continue;
        
        vec3 sampleViewPos = reconstructViewPos(offsetUV, sampleDepth);
        
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(viewPos.z - sampleViewPos.z));
        occlusion += (sampleViewPos.z >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }
    
    occlusion = 1.0 - (occlusion / float(kernelSize));
    fragColor = occlusion;
}
