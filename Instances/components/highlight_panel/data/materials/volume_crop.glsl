// Produces a vector that is > 0 within the crop, exactly 0 on the boundary, and < 0 outside for
// each component.
vec3 inCropVolume(float4x4 transformLocalToCrop, vec3 cropHalfExtents, vec3 localPos) {
    // Transform the local position into the crop volume coordinate space.
    vec3 cropPos = mulMat4x4Float3(transformLocalToCrop, localPos).xyz;
    // For each axis, subtract the half extent. The result is >0 outside the crop volume.
    vec3 t = abs(cropPos) - cropHalfExtents;
    // Negate to produce a vector that is > 0 within the crop, exactly 0 on the boundary, and
    // < 0 outside for each component.
    return -t;
}

float cropAlpha(vec3 inCropVolume) {
    return float(inCropVolume.x >= 0.0f && inCropVolume.y >= 0.0f && inCropVolume.z >= 0.0f);
}
