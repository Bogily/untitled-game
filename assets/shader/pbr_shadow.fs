#version 330

// PBR shader with shadow mapping support
in vec2 fragTexCoord;
in vec3 fragPosition;
in vec3 fragNormal;
in vec4 fragColor;

uniform sampler2D texture0;
uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;
uniform vec3 viewPos;
uniform vec3 lightDir;
uniform float shadowBias;

out vec4 finalColor;

// Fresnel-Schlick approximation
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// GGX/Towbridge-Reitz normal distribution function
float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265 * denom * denom;
    
    return nom / denom;
}

// Geometry function (Schlick-GGX)
float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return nom / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// Shadow calculation with PCF
float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    
    float shadow = 0.0;
    float bias = max(shadowBias * (1.0 - dot(normal, lightDir)), 0.005);
    
    // PCF (Percentage-Closer Filtering)
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    
    if(projCoords.z > 1.0)
        shadow = 0.0;
    
    return shadow;
}

void main()
{
    vec3 color = texture(texture0, fragTexCoord).rgb;
    
    // Material properties (simplified - can be extended to use textures)
    float metallic = 0.1;
    float roughness = 0.5;
    vec3 albedo = color;
    
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(viewPos - fragPosition);
    vec3 L = normalize(-lightDir);
    vec3 H = normalize(V + L);
    
    // Calculate shadows
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(fragPosition, 1.0);
    float shadow = calculateShadow(fragPosLightSpace, N, L);
    
    // Calculate radiance
    float distance = length(fragPosition - fragPosition);
    float attenuation = 1.0;
    vec3 radiance = vec3(1.0, 0.95, 0.9) * 2.0 * attenuation;
    
    // Cook-Torrance BRDF
    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    float NdotL = max(dot(N, L), 0.0);
    vec3 result = (kD * albedo / 3.14159265 + specular) * radiance * NdotL;
    
    // Apply shadow
    result *= (1.0 - shadow * 0.5);
    
    // Ambient
    result += albedo * 0.03;
    
    // HDR tonemapping
    result = result / (result + vec3(1.0));
    result = pow(result, vec3(1.0/2.2));
    
    finalColor = vec4(result, 1.0);
}
