#version 330

// Input vertex attributes
in vec3 vertexPosition;

// Input uniform values
uniform mat4 mvp;

// Output vertex attributes (to fragment shader)
out vec3 fragPosition;

void main()
{
    // Send vertex attributes to fragment shader
    fragPosition = vertexPosition;
    
    // Calculate final vertex position
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
