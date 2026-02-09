#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

// Uniforms
uniform mat4 mvp;
uniform mat4 model;

// Output to fragment shader
out vec2 fragTexCoord;
out vec3 fragPosition;  
out vec3 fragNormal;
out vec4 fragColor;

void main()
{
    fragTexCoord = vertexTexCoord;
    fragPosition = vec3(model * vec4(vertexPosition, 1.0));
    fragNormal = mat3(transpose(inverse(model))) * vertexNormal;
    fragColor = vertexColor;
    
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
