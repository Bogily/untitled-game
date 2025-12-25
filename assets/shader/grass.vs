#version 330 core
layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec2 vertexTexCoord;
layout (location = 2) in vec3 vertexNormal;
layout (location = 3) in vec4 vertexColor;

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
uniform float time;

uniform vec2 windDirection = vec2(1.0, 0.5);
uniform float windStrength = 0.5;
uniform float windSpeed = 2.0;

out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;
out float vHeight; // Pass height instead of recalculating in fragment shader

void main()
{
    float windEffect = vertexTexCoord.y;
    
    // Simplified wind calculation - removed one sin wave
    vec3 worldPos = vec3(matModel * vec4(vertexPosition, 1.0));
    float windWave = sin(time * windSpeed + worldPos.x * 0.5 + worldPos.z * 0.3);
    
    // Apply wind displacement
    vec3 windOffset = vec3(
        windDirection.x * windWave * windStrength * windEffect,
        0.0,
        windDirection.y * windWave * windStrength * windEffect
    );
    
    vec3 finalPosition = vertexPosition + windOffset;
    
    // Removed fragPosition calculation - not needed in fragment shader
    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 1.0)));
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    vHeight = vertexTexCoord.y; // Pass height for color gradient
    
    gl_Position = mvp * vec4(finalPosition, 1.0);
}