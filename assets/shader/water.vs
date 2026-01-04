#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
uniform float time;

// Output vertex attributes (to fragment shader)
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec3 fragNormal;
out vec3 worldPosition;
out vec3 viewSpaceNormal;

void main()
{
    // Subtle wave displacement - keep water mostly flat for realistic ocean/lake look
    vec3 pos = vertexPosition;
    
    // Gentle wave displacement (much smaller than before)
    float wave1 = sin(pos.x * 0.5 + time * 0.3) * 0.03;
    float wave2 = sin(pos.z * 0.7 - time * 0.25) * 0.025;
    float wave3 = sin((pos.x + pos.z) * 0.4 + time * 0.4) * 0.02;
    
    pos.y += wave1 + wave2 + wave3;
    
    // Calculate smooth normal from wave derivatives
    float dx = cos(pos.x * 0.5 + time * 0.3) * 0.5 * 0.03;
    dx += cos((pos.x + pos.z) * 0.4 + time * 0.4) * 0.4 * 0.02;
    
    float dz = cos(pos.z * 0.7 - time * 0.25) * 0.7 * 0.025;
    dz += cos((pos.x + pos.z) * 0.4 + time * 0.4) * 0.4 * 0.02;
    
    // Construct normal from derivatives
    vec3 normal = normalize(vec3(-dx, 1.0, -dz));
    
    // Transform to world space
    worldPosition = vec3(matModel * vec4(pos, 1.0));
    fragPosition = worldPosition;
    fragNormal = normalize(vec3(matNormal * vec4(normal, 0.0)));
    fragTexCoord = vertexTexCoord;
    viewSpaceNormal = normal;
    
    // Calculate final vertex position
    gl_Position = mvp * vec4(pos, 1.0);
}
