#version 330

// Color grading using 3D LUT (Look-Up Table)

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;    // Scene texture
uniform sampler3D lutTexture;  // 3D LUT texture for color grading
uniform float lutIntensity;    // Blend amount (0.0 = no effect, 1.0 = full effect)

out vec4 finalColor;

void main()
{
    // Sample the scene color
    vec4 color = texture(texture0, fragTexCoord);
    
    // Apply LUT only if intensity > 0
    if (lutIntensity > 0.0)
    {
        // Scale color to LUT space (assuming 32x32x32 LUT)
        // The LUT maps RGB input to RGB output
        vec3 lutCoord = clamp(color.rgb, 0.0, 1.0);
        
        // Sample the 3D LUT
        vec3 gradedColor = texture(lutTexture, lutCoord).rgb;
        
        // Blend between original and graded color
        color.rgb = mix(color.rgb, gradedColor, lutIntensity);
    }
    
    finalColor = color;
}
