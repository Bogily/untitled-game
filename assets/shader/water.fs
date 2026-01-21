#version 330

// Input vertex attributes (from vertex shader)
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec3 worldPosition;
in vec3 viewSpaceNormal;

// Input uniform values
uniform vec3 viewPos;
uniform vec3 lightDir;
uniform vec4 colDiffuse;
uniform float time;
uniform float normalScale;
uniform float foamThreshold;
uniform float foamIntensity;
uniform float glossiness;

// Output fragment color
out vec4 finalColor;

// Realistic water colors - tropical/clear water look
const vec3 deepWaterColor = vec3(0.02, 0.15, 0.25);     // Deep ocean blue
const vec3 mediumWaterColor = vec3(0.05, 0.35, 0.5);    // Medium depth blue-green
const vec3 shallowWaterColor = vec3(0.1, 0.55, 0.65);   // Shallow turquoise
const vec3 skyReflectionColor = vec3(0.5, 0.7, 0.9);    // Sky blue for reflections

// Simple hash function for better noise
float hash(vec2 p)
{
    p = fract(p * vec2(443.897, 441.423));
    p += dot(p, p + 19.19);
    return fract(p.x * p.y);
}

// Smoother noise function
float noise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f); // Smoothstep
    
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

void main()
{
    // Base normal from vertex shader
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(viewPos - fragPosition);
    vec3 L = normalize(-lightDir);
    
    // Layered normal perturbation for detailed ripples
    vec2 uv1 = fragTexCoord * 2.5 + vec2(time * 0.06, time * 0.04);
    vec2 uv2 = fragTexCoord * 6.0 - vec2(time * 0.035, time * 0.06);
    vec2 uv3 = fragTexCoord * 12.0 + vec2(time * 0.12, -time * 0.08);

    float n1 = noise(uv1) * 2.0 - 1.0;
    float n2 = noise(uv2) * 2.0 - 1.0;
    float n3 = noise(uv3) * 2.0 - 1.0;

    // Combine layered noise into a tangent-space normal perturbation
    vec3 ripplePerturb = vec3(n1 * 0.35 + n2 * 0.18 + n3 * 0.08, 0.0, n1 * 0.35 + n2 * 0.18 + n3 * 0.08);
    ripplePerturb *= normalScale; // uniform to control strength
    N = normalize(N + ripplePerturb);
    
    // Fresnel effect
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 5.0);
    fresnel = clamp(fresnel * 0.98, 0.01, 0.995); // slightly wider range
    
    // Distance-based depth effect (darker further down)
    float depthFade = smoothstep(-2.0, 2.0, worldPosition.y);
    vec3 waterColor = mix(deepWaterColor, shallowWaterColor, depthFade);
    waterColor = mix(waterColor, mediumWaterColor, 0.3);
    
    // Diffuse lighting (subtle, water is mostly specular)
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = waterColor * NdotL * 0.4;
    
    // Strong specular highlights (sun glints on water)
    vec3 H = normalize(L + V);
    float NdotH = max(dot(N, H), 0.0);
    float specular = pow(NdotH, max(16.0, glossiness));
    vec3 specularColor = vec3(1.0, 0.98, 0.95) * specular * 2.2;
    
    // Sky/environment reflection (simulated)
    vec3 reflectionColor = mix(skyReflectionColor, vec3(1.0), specular * 0.6);
    
    // Combine ambient + diffuse
    vec3 ambient = waterColor * 0.55;
    vec3 baseColor = ambient + diffuse;
    
    // Blend water color with reflections based on Fresnel
    vec3 color = mix(baseColor, reflectionColor, fresnel);
    
    // Add specular highlights on top
    color += specularColor;

    // Foam based on slope and small-scale noise
    float slope = length(vec2(dFdx(worldPosition.y), dFdy(worldPosition.y)));
    float foamMask = smoothstep(foamThreshold, foamThreshold * 3.0, slope);
    float foamNoise = noise(fragTexCoord * 8.0 + vec2(time * 0.1, -time * 0.08));
    foamMask *= smoothstep(0.2, 0.8, foamNoise) * foamIntensity;
    vec3 foamColor = vec3(1.0);
    color = mix(color, foamColor, foamMask);
    
    // Transparency based on viewing angle - more transparent when looking straight down
    float viewAngle = max(dot(N, V), 0.0);
    float alpha = mix(0.65, 0.98, fresnel); // More opaque at grazing angles
    
    // Ensure water looks good
    color = clamp(color, 0.0, 1.0);
    
    finalColor = vec4(color, alpha);
}
