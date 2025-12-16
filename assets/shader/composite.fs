#version 330 core

out vec4 fragColor;

in vec2 fragTexCoord;

uniform sampler2D sceneTexture;
uniform sampler2D ssaoTexture;

uniform int ssaoEnabled;
uniform float ssaoIntensity;

void main()
{
    // Use direct texture coordinates
    vec2 sceneUV = fragTexCoord;
    vec2 ssaoUV = fragTexCoord;
    
    vec3 sceneColor = texture(sceneTexture, sceneUV).rgb;
    
    if (ssaoEnabled == 1)
    {
        float ao = texture(ssaoTexture, ssaoUV).r;
        
        // Apply SSAO with intensity control
        // ssaoIntensity > 1 makes AO stronger, < 1 makes it weaker
        ao = pow(ao, ssaoIntensity);
        
        // Apply ambient occlusion to the scene
        sceneColor *= ao;
    }
    
    fragColor = vec4(sceneColor, 1.0);
}
