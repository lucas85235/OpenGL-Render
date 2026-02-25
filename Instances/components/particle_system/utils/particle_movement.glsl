float animationTime(float endValue)
{
    float currentTime = mod(materialParams.CurrentTime, endValue);
    float t = currentTime / endValue;
    return mix(0.0, endValue, t);
}

void ApplyGravity(inout float y)
{
    float mass = 1.0;
    float gravityFactor = 2.0;
    float freeFallConst = pow(animationTime(materialParams.Life),gravityFactor) * -9.807/2.0 * mass;
    y += freeFallConst;
}