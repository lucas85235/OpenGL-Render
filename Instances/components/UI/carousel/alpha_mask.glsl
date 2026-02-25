vec3 GetAlphaOnDistance(vec3 wpos, vec3 limit, float fadeBorder){
    float distToMinX = wpos.x + limit.x;
    float distToMaxX = limit.x - wpos.x;
    float distToMinY = wpos.y + limit.y;
    float distToMaxY = limit.y - wpos.y;
    float distToMinZ = wpos.z + limit.z;
    float distToMaxZ = limit.z - wpos.z;

    float fadeInX = clamp(distToMinX/fadeBorder, 0.0, 1.0);
    float fadeInY = clamp(distToMinY/fadeBorder, 0.0, 1.0);
    float fadeInZ = clamp(distToMinZ/fadeBorder, 0.0, 1.0);
    float fadeOutX = clamp(distToMaxX/fadeBorder, 0.0, 1.0);
    float fadeOutY = clamp(distToMaxY/fadeBorder, 0.0, 1.0);
    float fadeOutZ = clamp(distToMaxZ/fadeBorder, 0.0, 1.0);

    return vec3(min(fadeInX,fadeOutX),min(fadeInY,fadeOutY),min(fadeInZ,fadeOutZ));
}