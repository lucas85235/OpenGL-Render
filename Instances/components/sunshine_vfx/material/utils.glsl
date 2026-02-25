precision lowp float;

//=============================================================================
// Easing functions

// https://github.com/glslify/glsl-easings/blob/master/cubic-out.glsl
float cubicOut(float t) {
    float f = t - 1.0;
    return f * f * f + 1.0;
}

// https://github.com/glslify/glsl-easings/blob/master/quadratic-out.glsl
float quadraticOut(float t) {
    return -t * (t - 2.0);
}

// https://github.com/glslify/glsl-easings/blob/master/quartic-out.glsl
float quarticOut(float t) {
    return pow(t - 1.0, 3.0) * (1.0 - t) + 1.0;
}

// https://github.com/glslify/glsl-easings/blob/master/cubic-in-out.glsl
float cubicInOut(float t) {
    return t < 0.5
    ? 4.0 * t * t * t
    : 0.5 * pow(2.0 * t - 2.0, 3.0) + 1.0;
}

// https://github.com/glslify/glsl-easings/blob/master/quartic-in-out.glsl
float quarticInOut(float t) {
    return t < 0.5
    ? +8.0 * pow(t, 4.0)
    : -8.0 * pow(t - 1.0, 4.0) + 1.0;
}

// https://github.com/glslify/glsl-easings/blob/master/quadratic-in-out.glsl
float quadraticInOut(float t) {
    float p = 2.0 * t * t;
    return t < 0.5 ? p : -p + (4.0 * t) - 1.0;
}

//=============================================================================

// Utility functions
// Tiling and offset for UVs
vec2 tilingAndOffset(vec2 uv, vec2 tiling, vec2 offset) {
    return fract((uv + offset) * tiling);
}

// 3-color Gradient - Based on https://www.shadertoy.com/view/ttB3Rh
vec4 gradient(vec4 color1, vec4 color2, vec4 color3, float middlePoint, float position){
    vec4 firstMix = mix(color1, color2, position/middlePoint);
    vec4 secondMix = mix(color2, color3, (position - middlePoint)/(1.0 - middlePoint));
    vec4 col = mix(firstMix, secondMix, step(middlePoint, position));
    return col;
}

// Conversion from sRGB to linear
// https://google.github.io/filament/Materials.html#handlingcolors/linearcolors
float sRGB_to_linear(float color) {
    return color <= 0.04045 ? color / 12.92 : pow((color + 0.055) / 1.055, 2.4);
}

// https://google.github.io/filament/Materials.html#handlingcolors/pre-multipliedalpha
vec4 premultiplyAlpha(vec4 color)
{
    color.xyz *= color.a;
    return color;
}
