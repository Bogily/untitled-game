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
    
    // Add subtle normal perturbation for surface detail
    vec2 uv1 = fragTexCoord * 2.0 + vec2(time * 0.02, time * 0.015);
    vec2 uv2 = fragTexCoord * 3.5 - vec2(time * 0.015, time * 0.025);
    
    float n1 = noise(uv1) * 2.0 - 1.0;
    float n2 = noise(uv2) * 2.0 - 1.0;
    
    // Perturb normal slightly for surface ripples
    vec3 perturbedNormal = normalize(N + vec3(n1, 0.0, n2) * 0.15);
    N = perturbedNormal;
    
    // Fresnel effect - physically accurate
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 5.0);
    fresnel = clamp(fresnel * 0.98, 0.02, 0.98); // Clamp to realistic range
    
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
    float specular = pow(NdotH, 256.0);
    vec3 specularColor = vec3(1.0, 0.98, 0.95) * specular * 1.5;
    
    // Sky/environment reflection (simulated)
    vec3 reflectionColor = mix(skyReflectionColor, vec3(1.0), specular * 0.5);
    
    // Combine ambient + diffuse
    vec3 ambient = waterColor * 0.5;
    vec3 baseColor = ambient + diffuse;
    
    // Blend water color with reflections based on Fresnel
    vec3 color = mix(baseColor, reflectionColor, fresnel);
    
    // Add specular highlights on top
    color += specularColor;
    
    // Transparency based on viewing angle - more transparent when looking straight down
    float viewAngle = max(dot(N, V), 0.0);
    float alpha = mix(0.75, 0.95, fresnel); // More opaque at grazing angles
    
    // Ensure water looks good
    color = clamp(color, 0.0, 1.0);
    
    finalColor = vec4(color, alpha);
}
