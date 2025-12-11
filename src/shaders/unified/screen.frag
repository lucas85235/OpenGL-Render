#version 450

layout(set = 0, binding = 0) uniform sampler2D screenTexture;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 color = texture(screenTexture, fragTexCoord).rgb;
    outColor = vec4(color, 1.0);
}
