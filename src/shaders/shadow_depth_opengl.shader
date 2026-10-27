//====================================================================
// Shadow Depth Shader - Unified Format
// Depth-only pass from the POV of a directional light. Writes depth
// via gl_Position (clip-space z); no color outputs. Fragment stage is
// intentionally empty — the FBO has no color attachments.
// OpenGL only (matches ShadowPass backend constraint).
//====================================================================

#pragma shader_type(graphics)
#pragma opengl_only

//--------------------------------------------------------------------
// COMMON
//--------------------------------------------------------------------
#pragma begin(common)

// No shared helpers.

#pragma end(common)

//--------------------------------------------------------------------
// RESOURCES
//--------------------------------------------------------------------
#pragma begin(resources)

uniform mat4 u_light_space;
uniform mat4 u_model;

#pragma end(resources)

//--------------------------------------------------------------------
// VERTEX STAGE
//--------------------------------------------------------------------
#pragma begin(vertex)

layout(location = 0) in vec3 aPos;

void main() {
    gl_Position = u_light_space * u_model * vec4(aPos, 1.0);
}

#pragma end(vertex)

//--------------------------------------------------------------------
// FRAGMENT STAGE
//--------------------------------------------------------------------
#pragma begin(fragment)

// Empty: depth-only pass, no color attachment.
void main() {
}

#pragma end(fragment)
