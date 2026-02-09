#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D depthTexture;
uniform int flipY;

// Output fragment color
out vec4 finalColor;

void main()
{
    vec2 texCoord = fragTexCoord;
    
    
    // Sample the depth texture
    float depth = texture(depthTexture, texCoord).r;
    
    // Linearize depth for better visualization (approximate linear depth)
    // These values should match your camera's near and far planes
    float near = 0.1;
    float far = 100.0;
    float linearDepth = (2.0 * near) / (far + near - depth * (far - near));
    
    // Apply color gradient for depth visualization
    // Closer objects are darker, farther objects are lighter
    vec3 depthColor = vec3(linearDepth);
    
    // Output as white channel for visualization
    finalColor = vec4(depthColor, 1.0);
}
