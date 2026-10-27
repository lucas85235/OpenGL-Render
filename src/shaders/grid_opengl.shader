//====================================================================
// Grid Shader - Unified Format
// Infinite ground-plane grid (XZ). Fullscreen triangle vertex, fragment
// unprojects ray, intersects y=0 plane, computes grid lines with
// distance fade. X axis tinted red, Z axis tinted blue.
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

uniform mat4 u_view;
uniform mat4 u_projection;

#pragma end(resources)

//--------------------------------------------------------------------
// VERTEX STAGE
//--------------------------------------------------------------------
#pragma begin(vertex)

layout(location = 0) out vec3 nearPointWS;
layout(location = 1) out vec3 farPointWS;

vec3 unproject(vec2 ndc, float z, mat4 invViewProj) {
    vec4 p = invViewProj * vec4(ndc, z, 1.0);
    return p.xyz / p.w;
}

void main() {
    // Fullscreen triangle from gl_VertexID (no VBO required)
    vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    vec2 ndc = p * 2.0 - 1.0;

    mat4 invViewProj = inverse(u_projection * u_view);
    nearPointWS = unproject(ndc, -1.0, invViewProj);
    farPointWS  = unproject(ndc,  1.0, invViewProj);

    gl_Position = vec4(ndc, 0.0, 1.0);
}

#pragma end(vertex)

//--------------------------------------------------------------------
// FRAGMENT STAGE
//--------------------------------------------------------------------
#pragma begin(fragment)

layout(location = 0) in vec3 nearPointWS;
layout(location = 1) in vec3 farPointWS;

layout(location = 0) out vec4 outColor;

// Grid lines at given scale (world units between lines).
// Returns alpha in [0,1] from line coverage, using screen-space derivatives
// to keep lines ~1px wide regardless of distance.
float gridAlpha(vec2 worldXZ, float scale) {
    vec2 coord = worldXZ / scale;
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / max(derivative, vec2(1e-6));
    float line = min(grid.x, grid.y);
    return 1.0 - min(line, 1.0);
}

// Axis highlight: true if this fragment lies on X axis (z~0) or Z axis (x~0).
// Returns axis color contribution, blended into base line color.
vec3 axisTint(vec2 worldXZ, vec3 baseColor) {
    vec2 derivative = fwidth(worldXZ);
    // Distance to X axis = |z|; distance to Z axis = |x|. Measured in "pixels".
    float zInPx = abs(worldXZ.y) / max(derivative.y, 1e-6);
    float xInPx = abs(worldXZ.x) / max(derivative.x, 1e-6);

    vec3 color = baseColor;
    if (zInPx < 1.0) color = mix(color, vec3(0.85, 0.25, 0.25), 1.0 - zInPx); // X axis -> red
    if (xInPx < 1.0) color = mix(color, vec3(0.25, 0.45, 0.90), 1.0 - xInPx); // Z axis -> blue
    return color;
}

void main() {
    // Ray from near -> far. Intersect with plane y=0.
    vec3 dir = farPointWS - nearPointWS;
    // Avoid division by ~0 when ray is parallel to plane.
    if (abs(dir.y) < 1e-6) discard;

    float t = -nearPointWS.y / dir.y;
    if (t < 0.0 || t > 1.0) discard;

    vec3 worldPos = nearPointWS + t * dir;
    vec2 worldXZ = worldPos.xz;

    // Two scales: fine (1u) and coarse (10u). Coarse dominates far away.
    float fine   = gridAlpha(worldXZ, 1.0);
    float coarse = gridAlpha(worldXZ, 10.0);
    float alpha  = max(fine * 0.5, coarse);

    if (alpha <= 0.0) discard;

    // Distance fade: camera world position = row of inverse(view). Cheaper:
    // derive from the near-plane sample directly (nearPointWS approximates eye).
    vec3 camPos = (inverse(u_view) * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    float dist = length(worldPos - camPos);
    float fade = 1.0 - smoothstep(20.0, 120.0, dist);
    alpha *= fade;

    if (alpha <= 0.001) discard;

    vec3 lineColor = vec3(0.35);
    lineColor = axisTint(worldXZ, lineColor);

    // Write correct depth so the grid occludes / is occluded properly.
    vec4 clip = u_projection * u_view * vec4(worldPos, 1.0);
    float ndcZ = clip.z / clip.w;
    gl_FragDepth = ndcZ * 0.5 + 0.5;

    outColor = vec4(lineColor, alpha);
}

#pragma end(fragment)
