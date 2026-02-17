#version 430

layout(location = 0) in vec3 vertexPosition;
layout(location = 4) in vec4 instancePosDist; // xyz position, w signed distance

uniform mat4 matView;
uniform mat4 matProjection;
uniform float cubeScale;

out float vSignedDistance;

void main()
{
    vec3 worldPos = instancePosDist.xyz + vertexPosition * cubeScale;
    gl_Position = matProjection * matView * vec4(worldPos, 1.0);
    vSignedDistance = instancePosDist.w;
}
