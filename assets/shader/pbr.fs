#version 430 core

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;

out vec4 finalColor;

struct Light {
    int type;
    int enabled;
    vec2 pad0;
    vec4 positionRadius;
    vec4 color;
    float intensity;
    vec3 pad1;
};

uniform int numActiveLights;
uniform vec3 viewPos;
uniform vec3 ambientLight;

// Individual light uniforms (max 1024 lights - dynamically limited by GPU)
uniform int lights_type[1024];
uniform int lights_enabled[1024];
uniform vec4 lights_positionRadius[1024];
uniform vec4 lights_color[1024];
uniform float lights_intensity[1024];

uniform vec4 albedoColor;
uniform float metallicValue;
uniform float roughnessValue;

// Helpers
float DistributionGGX(vec3 N, vec3 H, float rough)
{
    float a = rough*rough;
    float a2 = a*a;
    float NdotH = max(dot(N,H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265 * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float rough)
{
    float r = (rough + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
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

    vec3 Lo = vec3(0.0);
    for (int i = 0; i < numActiveLights; ++i)
    {
        if (lights_enabled[i] == 0) continue;
        
        vec3 L;
        float attenuation;
        
        if (lights_type[i] == 2) {
            // Directional light
            L = normalize(-lights_positionRadius[i].xyz);
            attenuation = 1.0;
        } else {
            // Point light
            L = normalize(lights_positionRadius[i].xyz - fragPosition);
            float distance = length(lights_positionRadius[i].xyz - fragPosition);
            attenuation = 1.0 / (distance * distance);
        }
        
        vec3 H = normalize(V + L);

        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator    = NDF * G * F;
        float denom       = 4.0 * max(dot(N, V), 0.001) * max(dot(N, L), 0.001);
        vec3 specular = numerator / max(denom, 0.001);

        float NdotL = max(dot(N, L), 0.0);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        vec3 radiance = lights_color[i].rgb * lights_intensity[i] * attenuation;

        Lo += (kD * albedo / 3.14159265 + specular) * radiance * NdotL;
    }

    vec3 ambient = ambientLight * albedo;
    vec3 color = ambient + Lo;
    
    // Tone mapping - compress HDR to LDR
    color = color / (color + vec3(1.0));  // Reinhard tone mapping
    color = pow(color, vec3(1.0/2.2));   // Gamma correction
    
    finalColor = vec4(color, 1.0);
}
