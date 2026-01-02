#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;

// Output vertex attributes (to fragment shader)
out vec2 fragTexCoord;

void main()
{
    // Pass through texture coordinates
    fragTexCoord = vertexTexCoord;
    
    // Transform to clip space (fullscreen quad)
    gl_Position = vec4(vertexPosition, 1.0);
}
