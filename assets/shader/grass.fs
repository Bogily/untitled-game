#version 330 core
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;
in float vHeight;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Pre-normalized in code
uniform vec3 lightDir = vec3(0.3, -0.7, 0.5); 
uniform vec3 lightColor = vec3(1.0, 0.95, 0.8);

uniform vec3 grassColorTop = vec3(0.4, 0.7, 0.3);
uniform vec3 grassColorBottom = vec3(0.2, 0.4, 0.15);
uniform float ambientStrength = 0.3;

out vec4 finalColor;

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord);
    
    // Early discard for better performance on transparent pixels
    if (texelColor.a < 0.1)
        discard;
    
    // Grass color gradient - use passed height instead of recalculating
    vec3 grassColor = mix(grassColorBottom, grassColorTop, vHeight);
    
    // Simplified variation - removed expensive sin/dot
    float variation = fract(fragTexCoord.x * 43.5 + fragTexCoord.y * 31.3);
    grassColor *= (0.9 + variation * 0.2);
    
    // Lighting - normal already normalized in VS, lightDir should be pre-normalized
    float diff = max(dot(fragNormal, -lightDir), 0.0);
    
    // Simplified subsurface - reuse diff calculation
    float backlight = max(0.0, -diff * 0.5);
    
    // Combine lighting in one operation
    vec3 lighting = (ambientStrength + diff) * lightColor + vec3(0.3, 0.6, 0.2) * backlight * 0.3;
    
    // Final color
    vec3 result = grassColor * lighting * texelColor.rgb * fragColor.rgb;
    
    finalColor = vec4(result, texelColor.a * fragColor.a * colDiffuse.a);
}