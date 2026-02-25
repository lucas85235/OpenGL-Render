// Color Over Life Equal Time Calculus //
vec2 colorTransitionCalc(float currentTime, float totalTime, int numColors)
{
    int numTransitions = numColors -1;
    float transitionDuration = totalTime / float(numTransitions);
    float currentTransition = floor(currentTime / transitionDuration);
    currentTransition = clamp(currentTransition, 0.0, float(numTransitions-1));
    float localTime = currentTime - (currentTransition * transitionDuration);
    float mixFraction = localTime / transitionDuration;

    return vec2(currentTransition, mixFraction);
}

// Color Over Life Between 2 Colors //
vec4 colorOverLifeTime2(vec4 color0, vec4 color1, float currentTime, float totalTime)
{
    int numColors = 2;
    vec4 colors[2];
    colors[0] = color0;
    colors[1] = color1;

    vec2 transitionPrmts = vec2(colorTransitionCalc(currentTime, totalTime, numColors));
    int colorIndex = int(transitionPrmts[0]);

    return mix(colors[colorIndex], colors[colorIndex + 1], transitionPrmts[1]);
}

// Color Over Life Between 3 Colors //
vec4 colorOverLifeTime3(vec4 color0, vec4 color1, vec4 color2, float currentTime, float totalTime)
{
    int numColors = 3;
    vec4 colors[3];
    colors[0] = color0;
    colors[1] = color1;
    colors[2] = color2;

    vec2 transitionPrmts = vec2(colorTransitionCalc(currentTime, totalTime, numColors));
    int colorIndex = int(transitionPrmts[0]);

    return mix(colors[colorIndex], colors[colorIndex + 1], transitionPrmts[1]);
}

// Color Over Life Between 4 Colors //
vec4 colorOverLifeTime4(vec4 color0, vec4 color1, vec4 color2, vec4 color3, float currentTime, float totalTime)
{
    int numColors = 4;
    vec4 colors[4];
    colors[0] = color0;
    colors[1] = color1;
    colors[2] = color2;
    colors[3] = color3;

    vec2 transitionPrmts = vec2(colorTransitionCalc(currentTime, totalTime, numColors));
    int colorIndex = int(transitionPrmts[0]);

    return mix(colors[colorIndex], colors[colorIndex + 1], transitionPrmts[1]);
}

// Color Over Life Between 5 Colors //
vec4 colorOverLifeTime5(vec4 color0, vec4 color1, vec4 color2, vec4 color3, vec4 color4, float currentTime, float totalTime)
{
    int numColors = 5;
    vec4 colors[5];
    colors[0] = color0;
    colors[1] = color1;
    colors[2] = color2;
    colors[3] = color3;
    colors[4] = color4;

    vec2 transitionPrmts = vec2(colorTransitionCalc(currentTime, totalTime, numColors));
    int colorIndex = int(transitionPrmts[0]);

    return mix(colors[colorIndex], colors[colorIndex + 1], transitionPrmts[1]);
}

// Color Over Life Time By Percentage Between 2 Colors //
vec4 colorOverLifeTimeFract2(vec4 color0,float fract0, vec4 color1, float fract1 ,float currentTime, float totalTime)
{
    float l0 = fract0 * totalTime;
    float l1 = fract1 * totalTime;
    float animationTime = (fract1 - fract0) * totalTime;

    vec4 currentColor;

    // When the time is smaller then transition time
    if(currentTime <= l0)
    {
        currentColor = color0;
    }

    // Transition time is the difference between the two fractional values of time
    else if(currentTime > l0 && currentTime <= l1)
    {
        float endValue = animationTime;
        float timeNormalized = (currentTime - l0) / animationTime;
        currentColor = mix(color0, color1, timeNormalized);
    }

    // When time is bigger then transition time
    else
    {
        currentColor = color1;
    }

    return currentColor;
}

// Color Over Life Time By Percentage Between 3 Colors //
vec4 colorOverLifeTimeFract3(vec4 color0, float fract0, vec4 color1, vec2 fractMiddle, vec4 color2, float fract2 ,float currentTime, float totalTime)
{
    // Intervals times limit
    float l0 = fract0 * totalTime;
    float l1 = fractMiddle[0] * totalTime;
    float l2 = fractMiddle[1] * totalTime;
    float l3 = fract2 * totalTime;

    float firstAnimationTime = (fractMiddle[0] - fract0) * totalTime;
    float secondAnimationTime = (fract2 - fractMiddle[1])  * totalTime;

    float firstTransitionAmount = (l0 + firstAnimationTime);
    float secondTransitionAmount = (firstTransitionAmount + l2 + secondAnimationTime);
    vec4 currentColor;

    // When the time is smaller then transition time
    if(currentTime <= l0)
    {
        currentColor = color0;
    }

    // Transition time is the difference between the two fractional values of time
    else if(currentTime > l0 && currentTime <= l1)
    {
        float endValue = firstAnimationTime;
        float timeNormalized = (currentTime - l0) / firstAnimationTime;
        currentColor = mix(color0, color1, timeNormalized);
    }
    else if(currentTime > l1 && currentTime <= l2)
    {
        currentColor = color1;

    }
    else if(currentTime > l2 && currentTime <= l3)
    {
        float endValue = secondAnimationTime;
        float timeNormalized = (currentTime - l2) / secondAnimationTime;
        currentColor = mix(color1, color2, timeNormalized);
    }
    else //(currentTime > secondTransitionAmount + secondColorTime)
    {
        currentColor = color2;
    }
    return currentColor;
}

// make a smooth transition with a start transition time + animation time + end time
float alphaBlend(float deltaTime, float maxAnimationTime, float transitionDuration)
{
    float plateauStart = transitionDuration;
    float plateauEnd = maxAnimationTime - transitionDuration;
    float startPhase = smoothstep(0.0, transitionDuration, deltaTime);
    float endPhase = smoothstep(plateauEnd, maxAnimationTime, deltaTime);

    return startPhase * (1.0 - endPhase);
}