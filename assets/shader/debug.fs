#version 330 core

out vec4 fragColor;

in vec2 fragTexCoord;

uniform sampler2D texture0;
uniform sampler2D depthTexture;  // For normal reconstruction
uniform mat4 invProjection;
uniform vec2 screenSize;
uniform int mode; // 0 = color, 1 = depth, 2 = ssao, 3 = normals

const float nearPlane = 0.1;
const float farPlane = 1000.0;

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

// Reconstruct view-space position from depth (for normal view)
vec3 GetViewPos(vec2 screenUV, sampler2D depthTex)
{
    // Flip Y to sample from raylib's Y-flipped render texture
    vec2 sampleUV = vec2(screenUV.x, 1.0 - screenUV.y);
    float depth = texture(depthTex, sampleUV).r;
    
    vec2 ndc = screenUV * 2.0 - 1.0;
    float ndcZ = depth * 2.0 - 1.0;
    
    vec4 clipPos = vec4(ndc, ndcZ, 1.0);
    vec4 viewPos = invProjection * clipPos;
    viewPos /= viewPos.w;
    
    return viewPos.xyz;
}

vec3 GetNormalFromDepth(vec2 screenUV, sampler2D depthTex, vec2 texelSize)
{
    vec3 posCenter = GetViewPos(screenUV, depthTex);
    vec3 posRight = GetViewPos(screenUV + vec2(texelSize.x, 0.0), depthTex);
    vec3 posDown = GetViewPos(screenUV - vec2(0.0, texelSize.y), depthTex);
    vec3 posLeft = GetViewPos(screenUV - vec2(texelSize.x, 0.0), depthTex);
    vec3 posUp = GetViewPos(screenUV + vec2(0.0, texelSize.y), depthTex);
    
    vec3 dx = posRight - posLeft;
    vec3 dy = posUp - posDown;
    
    vec3 normal = normalize(cross(dx, dy));
    
    if (normal.z < 0.0)
        normal = -normal;
    
    return normal;
}

void main()
{
    if (mode == 0) // Color - raylib render texture is Y-flipped
    {
        vec2 uv = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);
        fragColor = texture(texture0, uv);
    }
    else if (mode == 1) // Depth - raylib render texture is Y-flipped
    {
        vec2 uv = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);
        float depth = texture(texture0, uv).r;
        float linearDepth = LinearizeDepth(depth) / farPlane;
        fragColor = vec4(vec3(linearDepth), 1.0);
    }
    else if (mode == 2) // SSAO - raw OpenGL FBO, NOT flipped
    {
        float ao = texture(texture0, fragTexCoord).r;
        fragColor = vec4(vec3(ao), 1.0);
    }
    else if (mode == 3) // Normals - reconstructed from depth
    {
        vec2 uv = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);
        float depth = texture(depthTexture, uv).r;
        
        if (depth >= 0.9999)
        {
            fragColor = vec4(0.5, 0.5, 1.0, 1.0); // Sky normal pointing up
        }
        else
        {
            vec2 texelSize = 1.0 / screenSize;
            vec3 normal = GetNormalFromDepth(fragTexCoord, depthTexture, texelSize);
            // Map normal from [-1,1] to [0,1] for visualization
            fragColor = vec4(normal * 0.5 + 0.5, 1.0);
        }
    }
    else
    {
        fragColor = texture(texture0, fragTexCoord);
    }
}
