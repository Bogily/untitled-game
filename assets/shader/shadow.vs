#version 330

// Shadow depth rendering vertex shader
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;

uniform mat4 mvp;
uniform mat4 matModel;

out vec3 fragPosition;
out vec3 fragNormal;

void main()
{
    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));
    fragNormal = normalize(mat3(transpose(inverse(matModel))) * vertexNormal);
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
