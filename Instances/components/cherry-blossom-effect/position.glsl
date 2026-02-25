highp float hash21(highp vec2 n, highp float index, highp float amount) {
    index = index/amount;
    float phase = index * 2.0 * 3.1415;
    return (sin(n.x * n.y * phase) + 1.0) / 2.0;
}

float alphaBlend(float animationTime, float maxAnimationTime, float transitionDuration){
    float plateauStart = transitionDuration;
    float plateauEnd = maxAnimationTime - transitionDuration;
    float startPhase = smoothstep(0.0,transitionDuration,animationTime);
    float endPhase = smoothstep(plateauEnd, maxAnimationTime, animationTime);

    return startPhase * (1.0 - endPhase);
}

vec3 getNewPosition(int index, float amount, float deltaTime, vec3 currentPosition, float animationTimeThreshold)
{
    vec3 position = currentPosition;
    float indexF = float(index);
    float animationTime = 20.0;
    vec3 size = vec3(10.0);
    vec2 velocity = vec2(3.0,6.0);
    float heightMultiplier = 0.2;
    float zMultiplier = 1.5;

    //random variables
    float random = mix(0.01,0.2,hash21(vec2(indexF), indexF, amount));
    float randomPos = mix(0.5,1.0,hash21(vec2(indexF), indexF, amount));
    float randomSize = mix(0.3,0.7,hash21(vec2(indexF), indexF, amount));

    // variant random time
    deltaTime = mod(deltaTime, animationTime * animationTimeThreshold);

    // scale the spheres to a random size
    position *= (size * randomSize);

    float smoothTime = max(min(log(deltaTime),1.0),0.0);
    float xPos = deltaTime * smoothTime;
    float yPos = deltaTime * smoothTime;
    float normalizedAnimationTime = deltaTime/animationTime;

    float phaseX = mix(0.1 * normalizedAnimationTime,normalizedAnimationTime * velocity.x,random);
    float phaseY = mix(normalizedAnimationTime * 0.5 - position.z,mix(normalizedAnimationTime * 0.9,normalizedAnimationTime,randomPos),random) * heightMultiplier * randomSize;
    float phaseZ = mix(normalizedAnimationTime * 0.2,mix(normalizedAnimationTime * 0.4,normalizedAnimationTime,randomPos),random) * zMultiplier * randomSize;
    position.x += xPos * phaseX  + normalizedAnimationTime;
    position.y += (sin(yPos * phaseY) + 1.0)/2.0 * phaseY + normalizedAnimationTime;
    position.z += (sin(yPos * phaseZ * 0.2) + 1.0)/2.0 * phaseZ;

    return position;
}