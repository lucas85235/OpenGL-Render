//====================================================================
// Tonemap Shader - Unified Format
// Final-pass HDR -> LDR conversion. ACES Narkowicz filmic fit +
// gamma correction + exposure. Fullscreen triangle via gl_VertexID.
// OpenGL only.
//====================================================================

#pragma shader_type(graphics)
#pragma opengl_only

#pragma begin(common)
#pragma end(common)

#pragma begin(resources)

@binding(0) uniform sampler2D u_source;

uniform float u_exposure;
uniform float u_gamma;

#pragma end(resources)

#pragma begin(vertex)

layout(location = 0) out vec2 fragTexCoord;

void main() {
    vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    fragTexCoord = p;
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}

#pragma end(vertex)

#pragma begin(fragment)

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

// ACES Narkowicz — cheap filmic approximation, industry-standard default.
vec3 acesNarkowicz(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec3 hdr = texture(u_source, fragTexCoord).rgb;
    vec3 mapped = acesNarkowicz(hdr * u_exposure);
    vec3 corrected = pow(mapped, vec3(1.0 / max(u_gamma, 0.01)));
    outColor = vec4(corrected, 1.0);
}

#pragma end(fragment)
