#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Output fragment color
out vec4 finalColor;

// Uniform inputs
uniform float uTime;
uniform vec3 uFlameColor;      // Base flame color
uniform float uIntensity;      // Intensity (0.0 = off, 1.0 = full)

// Noise function for organic flame effect
float hash(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// Fractal Brownian Motion for detailed flames
float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    
    for (int i = 0; i < 5; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    
    return value;
}

void main() {
    // Early exit if intensity is zero
    if (uIntensity <= 0.0) {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    
    // Use texture coordinates
    vec2 uv = fragTexCoord;
    vec2 p = uv;
    
    // Center the flame horizontally, keep it at bottom
    p.x = (p.x - 0.5) * 2.0;
    
    // Flip Y so flame goes upward
    p.y = 1.0 - p.y;
    
    // Create flame shape - narrower at top, wider at bottom
    float width = 0.3 + p.y * 0.4;
    float shape = 1.0 - smoothstep(0.0, width, abs(p.x));
    
    // Animate the flame with time
    vec2 q = p;
    q.x += 0.1 * sin(uTime * 2.0 + p.y * 8.0);
    q.y += uTime * 0.3;
    
    // Create flame turbulence
    float n1 = fbm(q * 3.0);
    float n2 = fbm(q * 5.0 + vec2(uTime * 0.5));
    
    // Combine noise for flame effect
    float flame = n1 * 0.6 + n2 * 0.4;
    
    // Make flame more intense at bottom, fade at top
    flame *= (1.0 - p.y * 0.8);
    
    // Apply shape mask
    flame *= shape;
    
    // Add flickering
    float flicker = 0.95 + 0.05 * sin(uTime * 15.0 + n1 * 10.0);
    flame *= flicker;
    
    // Create color gradient from hot (bottom) to cool (top)
    vec3 hotColor = uFlameColor;
    vec3 warmColor = uFlameColor * 0.7 + vec3(0.3, 0.15, 0.0);
    vec3 coolColor = uFlameColor * 0.3 + vec3(0.1, 0.0, 0.0);
    
    // Mix colors based on height
    vec3 color = mix(coolColor, warmColor, flame);
    color = mix(color, hotColor, flame * (1.0 - p.y));
    
    // Apply intensity
    color *= uIntensity;
    
    // Output final color
    finalColor = vec4(color, 1.0);
}
