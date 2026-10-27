//====================================================================
// Outline Shader - Unified Format
// Inflated-shell outline. Vertex pushes position along normal by
// u_outline_width (object space). Fragment emits solid color.
// Pipeline runs with cull=Front + depth write OFF so only the back
// faces of the inflated shell leak past the original mesh silhouette.
// OpenGL only (non-block uniforms).
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

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform float u_outline_width;
uniform vec4 u_outline_color;

#pragma end(resources)

//--------------------------------------------------------------------
// VERTEX STAGE
//--------------------------------------------------------------------
#pragma begin(vertex)

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

void main() {
    vec3 inflated = aPos + normalize(aNormal) * u_outline_width;
    gl_Position = u_projection * u_view * u_model * vec4(inflated, 1.0);
}

#pragma end(vertex)

//--------------------------------------------------------------------
// FRAGMENT STAGE
//--------------------------------------------------------------------
#pragma begin(fragment)

layout(location = 0) out vec4 outColor;

void main() {
    outColor = u_outline_color;
}

#pragma end(fragment)
