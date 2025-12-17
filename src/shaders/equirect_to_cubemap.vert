#version 450

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 localPos;

uniform mat4 projection;
uniform mat4 view;

void main() {
    localPos = inPosition;
    gl_Position = projection * view * vec4(inPosition, 1.0);
}
