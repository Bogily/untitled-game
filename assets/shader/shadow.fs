#version 330

// Shadow depth rendering - renders to depth texture
in vec3 fragPosition;
in vec3 fragNormal;

uniform mat4 lightSpaceMatrix;

void main()
{
    // The depth is automatically written to gl_FragDepth
    // We just need to output the position for verification
    gl_FragDepth = gl_FragCoord.z;
}
