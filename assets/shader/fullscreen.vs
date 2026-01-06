#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;

// Output vertex attributes (to fragment shader)
out vec2 fragTexCoord;
out vec4 fragColor;

void main()
{
    // Pass through texture coordinates and color
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    
    // Transform to clip space (fullscreen quad)
    gl_Position = vec4(vertexPosition, 1.0);
}
