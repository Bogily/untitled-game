#version 430 core

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;

out vec4 finalColor;

#define TILE_SIZE 16
#define MAX_LIGHTS_PER_TILE 256

struct Light {
    int type;           // 1 = point light, 2 = directional light
    int enabled;
    vec3 position;      // For point lights OR direction for directional lights
    float radius;       // Light radius (point lights only)
    vec4 color;
    float intensity;
    vec2 padding;
};

// Shader storage buffers
layout(std430, binding = 0) buffer LightBuffer {
    Light lights[];
};

layout(std430, binding = 1) buffer VisibleLightIndicesBuffer {
    uint visibleLightIndices[];
};

layout(std430, binding = 2) buffer LightGridBuffer {
    uvec2 lightGrid[];  // x = offset, y = count
};

uniform int numLights;
uniform vec3 viewPos;
uniform vec2 screenSize;

uniform vec4 albedoColor;
uniform float metallicValue;
uniform float roughnessValue;

// PBR Functions
float DistributionGGX(vec3 N, vec3 H, float rough)
{
    float a = rough * rough;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265 * denom * denom;

    return nom / max(denom, 0.001);
}

float GeometrySchlickGGX(float NdotV, float rough)
{
    float r = (rough + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / max(denom, 0.001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float rough)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, rough);
    float ggx1 = GeometrySchlickGGX(NdotL, rough);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(viewPos - fragPosition);
    vec3 albedo = albedoColor.rgb;
    float metallic = metallicValue;
    float roughness = max(0.05, roughnessValue);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // Calculate tile index from fragment position
    ivec2 tileID = ivec2(gl_FragCoord.xy) / TILE_SIZE;
    uint numTilesX = uint((screenSize.x + TILE_SIZE - 1) / TILE_SIZE);
    uint tileIndex = tileID.y * numTilesX + tileID.x;

    // Get visible light range for this tile
    uint lightOffset = lightGrid[tileIndex].x;
    uint lightCount = lightGrid[tileIndex].y;

    vec3 Lo = vec3(0.0);

    // Iterate over visible lights for this tile
    for (uint i = 0; i < lightCount; i++)
    {
        uint lightIndex = visibleLightIndices[lightOffset + i];
        
        if (lightIndex >= numLights || lights[lightIndex].enabled == 0)
            continue;

        vec3 L;
        float attenuation;

        if (lights[lightIndex].type == 2) {
            // Directional light (sun)
            L = normalize(-lights[lightIndex].position);
            attenuation = 1.0;
        } else {
            // Point light
            L = normalize(lights[lightIndex].position - fragPosition);
            float distance = length(lights[lightIndex].position - fragPosition);
            attenuation = 1.0 / max(distance * distance, 0.01);
        }

        vec3 H = normalize(V + L);

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denom = 4.0 * max(dot(N, V), 0.001) * max(dot(N, L), 0.001);
        vec3 specular = numerator / max(denom, 0.001);

        float NdotL = max(dot(N, L), 0.0);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        vec3 radiance = lights[lightIndex].color.rgb * lights[lightIndex].intensity * attenuation;

        Lo += (kD * albedo / 3.14159265 + specular) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.001) * albedo;
    vec3 color = ambient + Lo;

    finalColor = vec4(color, 1.0);
}
