#version 330 core

layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gAlbedo;

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;

uniform sampler2D texture0;  // Albedo/diffuse texture
uniform vec4 colDiffuse;     // Diffuse color

void main()
{
    // Store fragment position in view space
    gPosition = vec4(fragPosition, 1.0);
    
    // Store normal in view space
    gNormal = vec4(normalize(fragNormal), 1.0);
    
    // Store albedo color
    vec4 texColor = texture(texture0, fragTexCoord);
    gAlbedo = texColor * colDiffuse;
}
