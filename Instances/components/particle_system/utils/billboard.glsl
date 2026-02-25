vec3 billboard(vec3 cameraPosition, vec3 particlePosition, vec3 vertexPosition, float size) {

    vec3 d = normalize(cameraPosition - particlePosition);
    vec3 worldUp = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(worldUp, d));
    vec3 upPrime = cross(d, right);
    mat3 rot = mat3(right, upPrime, d);
    return rot * (vertexPosition * size);
}
