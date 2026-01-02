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

// Simple hash function for pseudo-random numbers
float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

// Smooth noise function
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

// Fractional Brownian Motion for clouds
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
    
    // Generate clouds using FBM
    vec2 cloudUV = direction.xz / (direction.y + 0.5) * 0.5;
    cloudUV += time * 0.01; // Slow cloud movement
    
    float cloudNoise = fbm(cloudUV * 3.0);
    
    // Create cloud shapes
    float clouds = smoothstep(0.4, 0.6, cloudNoise);
    clouds *= smoothstep(-0.1, 0.3, direction.y); // Fade out clouds below horizon
    
    // Use uniform cloud color
    
    // Add some variation to clouds
    float cloudDetail = fbm(cloudUV * 8.0);
    clouds *= mix(0.8, 1.0, cloudDetail);
    
    // Blend sky and clouds
    vec3 finalSkyColor = mix(finalSkyColorBase, cloudColor, clouds * 0.7);
    
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
