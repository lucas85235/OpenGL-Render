// GLSL functions for defining signed distance fields.
// ----------------------------------------------------------
// Hover and select

float circleSize = 0.30;
float circleSelectedSize = 0.20;

highp vec4 hoverAndSelectPanelMask(highp vec2 fragCoord,
                             highp vec4 myPanelColor,
                             highp vec4 myBorderColor,
                             highp vec2 cursorPosition,
                             bool isSelected,
                             highp float lerpTime,
                             highp float controlSelectEffect,
                             highp float changeStateLerp,
                             highp float circleRadius) {
    highp vec2 posCircle = fragCoord;
    // SysUI version: highp vec2 circleCenter = vec2(cursorPosition.x * 0.7, cursorPosition.y * 0.6); // todo: remove magic numbers
    highp vec2 circleCenter = vec2(cursorPosition.x * 0.45, cursorPosition.y * -0.45); // todo: remove magic numbers

    circleSize = circleRadius;
    circleSelectedSize = circleRadius / 1.5f;

    changeStateLerp = clamp(changeStateLerp, 0.0f, 1.0f);
    circleSize = mix(circleSize, circleSelectedSize, changeStateLerp);

    // Gets the panel color
    highp float maskSmoothness = 0.5;
    highp float smoothClamp = clamp(0.0, circleSize - 0.15, maskSmoothness);
    highp float smoothClamp2 = clamp(0.0, circleSize - 0.08, maskSmoothness);

    highp float circleMask = smoothstep(circleSize, smoothClamp, length(posCircle - circleCenter));
    highp float circleMask2 = smoothstep(circleSize - (circleSize * 0.1), smoothClamp, length(posCircle - circleCenter));

    // Apply mask on the panel
    circleMask = mix(circleMask, 1.0, lerpTime);
    circleMask2 = mix(circleMask2, 1.0, lerpTime);

    highp vec4 color = mix(vec4(0.0), myPanelColor, circleMask);
    color += mix(vec4(0.0), myBorderColor, circleMask2);

    if (isSelected) {
        highp float maxSize = circleSelectedSize;
        highp float dist = length(posCircle - circleCenter);
        highp float thickness = 2.0;

        highp float radiusEffect = controlSelectEffect * maxSize * thickness;
        highp float alpha = clamp(0.7 - controlSelectEffect, 0.0, 0.5);
        highp vec4 colorPulse = vec4(0.0);
        highp vec4 colorTemp = color;

        // Sets the color of the fragment: white if inside the thickness of the circle, transparent if outside
        if (dist < radiusEffect) {
            colorPulse.a = color.a * alpha; // White opaque

            if (alpha >= 0.5) {
                color = vec4(0.5, 0.5, 1.0, color.a);
            }
        }

        return colorTemp + smoothstep(0.0,0.9,(color * colorPulse.a));
    }

    return color;
}

// ----------------------------------------------------------
// Box, line and arc line

highp float roundedBoxSDF(in highp vec2 pos, in highp vec2 rectSize, in highp float cornerRadius) {
    highp vec2 dist = abs(pos) - rectSize;
    return length(max(dist,0.0)) + min(max(dist.x,dist.y), 0.0) - cornerRadius;
}

highp float lineDistanceFunction(in highp vec2 pos, in highp vec2 start, in highp vec2 end, highp float width) {
    highp vec2 dir = start - end;
    highp float size = length(dir);
    dir /= size;
    highp vec2 proj = max(0.0, min(size, dot((start - pos), dir))) * dir;
    return length((start - pos) - proj) -  (width / 2.0);
}

highp float verticalLineDistanceFunction(in highp vec2 pos,
                                   in highp vec2 rectSize,
                                   in highp float cornerRadius,
                                   highp float borderThickness,
                                   highp float linePercent) {
    if (linePercent <= 0.0001)
    return 0.0;

    linePercent = clamp(linePercent, 0.0, 1.0);

    highp vec2 halfRectSize = rectSize * 0.5;
    highp vec2 start = vec2(halfRectSize.x, (halfRectSize.y - cornerRadius) * (1.0 - linePercent));
    highp vec2 end = halfRectSize - vec2(0, cornerRadius);

    highp float verticalLine = lineDistanceFunction(pos, start, end, borderThickness);
    return verticalLine;
}

highp float horizontalLineDistanceFunction(in highp vec2 pos,
                                     in highp vec2 rectSize,
                                     in highp float cornerRadius,
                                     highp float borderThickness,
                                     highp float linePercent) {
    if (linePercent <= 0.0001)
    return 0.0;

    linePercent = clamp(linePercent, 0.0, 1.0);

    highp vec2 halfRectSize = rectSize * 0.5;
    highp vec2 start = vec2((halfRectSize.x - cornerRadius) * (1.0 - linePercent), halfRectSize.y);
    highp vec2 end = halfRectSize - vec2(cornerRadius, 0);

    highp float horizontalLine = lineDistanceFunction(pos, start, end, borderThickness);
    return horizontalLine;
}

highp float cornerArcDistanceFunction(in highp vec2 pos,
                                in highp vec2 rectSize,
                                in highp float cornerRadius,
                                in highp float borderThickness,
                                in highp float arcPercent) {
    highp vec2 circleCenter = rectSize * 0.5 - cornerRadius;
    highp vec2 toCenterDelta = pos - circleCenter;

    highp float quarterAngle = radians(45.0);
    highp float angle = atan(toCenterDelta.y / abs(toCenterDelta.x));

    if (toCenterDelta.x < 0.0)
    angle = radians(180.0) - angle;

    angle = clamp(angle, quarterAngle - quarterAngle * arcPercent, quarterAngle + quarterAngle * arcPercent);
    highp vec2 arcPoint = vec2(cos(angle), sin(angle)) * cornerRadius;

    return length(arcPoint - toCenterDelta) - borderThickness * 0.5;
}

highp float distanceFunctionUnion(highp float a, highp float b) {
    return min(a, b);
}

highp float inverseLerp(highp float value, highp float pA, highp float pB) {
    return (value - pA) / abs(pA - pB);
}

highp float roundedCornerRectBorderMask(in highp vec2 inPanelWorldSpaceCoords,
                                  in highp vec2 panelRectSize,
                                  highp float cornerRadius,
                                  highp float borderThickness,
                                  highp float lineSpread,
                                  highp float arcSpread) {
    highp float verticalLine = verticalLineDistanceFunction(inPanelWorldSpaceCoords, panelRectSize, cornerRadius, borderThickness, lineSpread);
    highp float horizontalLine = horizontalLineDistanceFunction(inPanelWorldSpaceCoords, panelRectSize, cornerRadius, borderThickness, lineSpread);

    highp float mask = distanceFunctionUnion(verticalLine, horizontalLine);
    highp float arc = cornerArcDistanceFunction(inPanelWorldSpaceCoords, panelRectSize, cornerRadius, borderThickness, arcSpread);

    mask = distanceFunctionUnion(mask, arc);
    return mask;
}