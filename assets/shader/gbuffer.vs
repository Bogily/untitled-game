#version 330 core

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec2 vertexTexCoord;
layout (location = 2) in vec3 vertexNormal;

out vec3 fragPosition;
out vec2 fragTexCoord;
out vec3 fragNormal;

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matView;
uniform mat4 matNormal;

void main()
{
    // Transform position to view space (required for SSAO)
    vec4 viewPos = matView * matModel * vec4(vertexPosition, 1.0);
    fragPosition = viewPos.xyz;
    
    // Pass through texture coordinates
    fragTexCoord = vertexTexCoord;
    
    // Transform normal to view space
    mat3 normalMatrix = transpose(inverse(mat3(matView * matModel)));
    fragNormal = normalize(normalMatrix * vertexNormal);
    
    // Final position
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
