#version 330 core

// Vertex attributes
layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec2 vertexTexCoord;
layout (location = 2) in vec3 vertexNormal;
layout (location = 3) in vec4 vertexColor;

// Instance attributes (per-instance data)
layout (location = 4) in vec4 instanceData; // xyz = position, w = scale
layout (location = 5) in vec4 instanceWind; // xy = windOffset.xz, zw unused

// Uniforms
uniform mat4 mvp;
uniform mat4 matView;
uniform mat4 matProjection;
uniform float time;
uniform vec3 viewPos;

// Wind parameters
uniform vec2 windDirection;
uniform float windStrength;
uniform float windSpeed;

// Output to fragment shader
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;

void main()
{
    vec3 instancePos = instanceData.xyz;
    float instanceScale = instanceData.w;
    
    // Get camera right vector from view matrix for billboarding
    vec3 camRight = vec3(matView[0][0], matView[1][0], matView[2][0]);
    vec3 camUp = vec3(0.0, 1.0, 0.0); // Keep grass upright
    
    // Scale the vertex
    vec3 scaledVertex = vertexPosition * instanceScale;
    
    // Billboard: transform local X to camera right, keep Y as world up
    vec3 billboardPos = instancePos;
    billboardPos += camRight * scaledVertex.x;
    billboardPos.y += scaledVertex.y;
    
    // Wind animation provided per-instance in instanceWind.xy (precomputed by CPU or compute shader)
    float heightFactor = vertexTexCoord.y; // 0 at bottom, 1 at top
    vec3 windOffset = vec3(instanceWind.x * (heightFactor * heightFactor), 0.0, instanceWind.y * (heightFactor * heightFactor));
    billboardPos += windOffset;
    
    // Calculate final position
    gl_Position = matProjection * matView * vec4(billboardPos, 1.0);
    
    // Pass data to fragment shader
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    
    // Normal facing camera
    vec3 camForward = normalize(viewPos - instancePos);
    fragNormal = normalize(mix(camUp, camForward, 0.3));
}
