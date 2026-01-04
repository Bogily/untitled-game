#version 330

// Input vertex attributes (from vertex shader)
in vec3 fragPosition;

// Output fragment color
out vec4 finalColor;

// Uniform values
uniform float time;
uniform vec3 skyColor;
uniform vec3 cloudColor;
uniform vec3 sunDirection;
uniform vec3 sunColor;

// 3D Cloud uniforms
uniform float cloudDensity;      // Overall cloud thickness/opacity (0.0-1.0)
uniform float cloudHeight;       // Cloud layer altitude (world units)
uniform float cloudScale;        // Cloud detail scale (larger = bigger clouds)
uniform float cloudSpeed;        // Animation speed multiplier
uniform float cloudCoverage;     // Cloud coverage amount (0.0-1.0)
uniform vec3 cloudOffset;        // 3D offset for cloud animation

// Simple hash function for pseudo-random numbers
float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

// 3D hash for 3D noise
float hash3D(vec3 p)
{
    return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453123);
}

// Smooth noise function (2D)
float noise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    
    // Smooth interpolation
    f = f * f * (3.0 - 2.0 * f);
    
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// 3D Perlin-like noise
float noise3D(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);
    
    // Smooth interpolation
    f = f * f * (3.0 - 2.0 * f);
    
    // Sample 8 corners of cube
    float n000 = hash3D(i + vec3(0.0, 0.0, 0.0));
    float n100 = hash3D(i + vec3(1.0, 0.0, 0.0));
    float n010 = hash3D(i + vec3(0.0, 1.0, 0.0));
    float n110 = hash3D(i + vec3(1.0, 1.0, 0.0));
    float n001 = hash3D(i + vec3(0.0, 0.0, 1.0));
    float n101 = hash3D(i + vec3(1.0, 0.0, 1.0));
    float n011 = hash3D(i + vec3(0.0, 1.0, 1.0));
    float n111 = hash3D(i + vec3(1.0, 1.0, 1.0));
    
    // Trilinear interpolation
    float nx00 = mix(n000, n100, f.x);
    float nx01 = mix(n001, n101, f.x);
    float nx10 = mix(n010, n110, f.x);
    float nx11 = mix(n011, n111, f.x);
    
    float nxy0 = mix(nx00, nx10, f.y);
    float nxy1 = mix(nx01, nx11, f.y);
    
    return mix(nxy0, nxy1, f.z);
}

// 3D Fractional Brownian Motion for volumetric clouds
float fbm3D(vec3 p)
{
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    
    for (int i = 0; i < 4; i++)
    {
        value += amplitude * noise3D(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    
    return value;
}

// Fractional Brownian Motion for 2D clouds (legacy)
float fbm(vec2 p)
{
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    
    for (int i = 0; i < 5; i++)
    {
        value += amplitude * noise(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    
    return value;
}

// Raymarching function for volumetric clouds
float sampleCloudDensity(vec3 pos)
{
    // Apply cloud offset for animation
    vec3 samplePos = pos + cloudOffset;
    
    // Base cloud shape using 3D FBM
    float cloudShape = fbm3D(samplePos * cloudScale);
    
    // Add detail noise at higher frequency
    float detail = noise3D(samplePos * cloudScale * 3.0) * 0.3;
    cloudShape += detail;
    
    // Use coverage to threshold clouds
    cloudShape = smoothstep(1.0 - cloudCoverage, 1.0, cloudShape);
    
    return cloudShape * cloudDensity;
}

void main()
{
    // Normalize the position to use as direction
    vec3 direction = normalize(fragPosition);
    
    // Sky gradient based on vertical direction
    float skyGradient = direction.y * 0.5 + 0.5; // Remap from [-1,1] to [0,1]
    
    // Sky colors - use uniform skyColor for top, lighter version for horizon
    vec3 skyTop = skyColor;
    vec3 skyHorizon = skyColor * 1.3; // Lighter version at horizon
    vec3 finalSkyColorBase = mix(skyHorizon, skyTop, pow(skyGradient, 0.7));
    
    // Add some atmosphere glow near horizon
    float horizonGlow = 1.0 - abs(direction.y);
    horizonGlow = pow(horizonGlow, 3.0);
    finalSkyColorBase += vec3(0.8, 0.7, 0.5) * horizonGlow * 0.3;
    
    // === 3D VOLUMETRIC CLOUDS ===
    float cloudAlpha = 0.0;
    vec3 volumetricCloudColor = cloudColor;
    
    // Only raymarch for rays pointing above horizon
    if (direction.y > 0.0)
    {
        // Define cloud layer bounds
        float cloudLayerStart = cloudHeight;
        float cloudLayerEnd = cloudHeight + 40.0; // Cloud layer thickness
        
        // Calculate ray-plane intersection for cloud layer
        // Ray origin at camera (0,0,0), ray direction is 'direction'
        float tStart = cloudLayerStart / max(direction.y, 0.001);
        float tEnd = cloudLayerEnd / max(direction.y, 0.001);
        
        // Raymarch through cloud layer
        const int numSteps = 16; // Fewer steps for performance
        float stepSize = (tEnd - tStart) / float(numSteps);
        float transmittance = 1.0;
        vec3 scatteredLight = vec3(0.0);
        
        for (int i = 0; i < numSteps; i++)
        {
            float t = tStart + stepSize * (float(i) + 0.5);
            vec3 samplePos = direction * t;
            
            // Sample cloud density at this position
            float density = sampleCloudDensity(samplePos * 0.01); // Scale down for larger clouds
            
            if (density > 0.001)
            {
                // Simple lighting - more light from sun direction
                vec3 sunDir = normalize(sunDirection);
                float sunDot = max(0.0, dot(normalize(samplePos), sunDir));
                float lighting = mix(0.7, 1.2, pow(sunDot, 1.5)); // Brighter, less harsh falloff
                
                // Accumulate light scattering
                float densityStep = density * stepSize * 0.1;
                scatteredLight += transmittance * densityStep * cloudColor * lighting * 1.5; // Boost brightness
                transmittance *= exp(-densityStep * 1.5); // Reduced extinction for brighter clouds
                
                // Early exit if cloud is opaque
                if (transmittance < 0.01) break;
            }
        }
        
        cloudAlpha = 1.0 - transmittance;
        volumetricCloudColor = scatteredLight + cloudColor * transmittance * 0.4; // Higher ambient contribution
    }
    
    // === LEGACY 2D CLOUDS (for horizon/backup) ===
    vec2 cloudUV = direction.xz / (direction.y + 0.5) * 0.5;
    cloudUV += time * 0.01; // Slow cloud movement
    
    float cloudNoise = fbm(cloudUV * 3.0);
    
    // Create cloud shapes
    float clouds2D = smoothstep(0.4, 0.6, cloudNoise);
    clouds2D *= smoothstep(-0.1, 0.3, direction.y); // Fade out clouds below horizon
    
    // Add some variation to clouds
    float cloudDetail = fbm(cloudUV * 8.0);
    clouds2D *= mix(0.8, 1.0, cloudDetail);
    
    // Blend 2D and 3D clouds (use 2D near horizon, 3D above)
    float blendFactor = smoothstep(0.0, 0.3, direction.y);
    float finalCloudAlpha = mix(clouds2D * 0.7, cloudAlpha, blendFactor);
    vec3 finalCloudColor = mix(cloudColor, volumetricCloudColor, blendFactor);
    
    // Blend sky and clouds
    vec3 finalSkyColor = mix(finalSkyColorBase, finalCloudColor, finalCloudAlpha);
    
    // Add sun glow using uniform sun direction and color
    vec3 sunDir = normalize(sunDirection);
    float sun = max(0.0, dot(direction, sunDir));
    
    // Large sun disc
    float sunDisc = pow(sun, 128.0) * 2.0;
    // Sun glow/corona
    float sunGlow = pow(sun, 16.0) * 0.5;
    
    finalSkyColor += sunColor * (sunDisc + sunGlow);
    
    finalColor = vec4(finalSkyColor, 1.0);
}
