#version 450

uniform mat4 projection;
uniform mat4 view;

layout(location = 0) in vec3 inPosition;

out vec3 fragTexCoord;

void main() {
    fragTexCoord = inPosition;
    vec4 pos = projection * view * vec4(inPosition, 1.0);
    // Set z = w for maximum depth (drawn behind everything)
    gl_Position = pos.xyww;
}
