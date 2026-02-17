#version 430

in float vSignedDistance;

uniform float shellThreshold;

out vec4 fragColor;

void main()
{
    float intensity = 1.0 - clamp(abs(vSignedDistance) / max(shellThreshold, 1e-6), 0.0, 1.0);
    vec3 baseColor = (vSignedDistance < 0.0)
        ? vec3(1.0, 0.275, 0.275)
        : vec3(0.314, 0.706, 1.0);
    float alpha = 0.35 + 0.65 * intensity;
    fragColor = vec4(baseColor, alpha);
}
