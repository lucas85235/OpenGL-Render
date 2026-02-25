// GLSL functions for defining signed distance fields.

float roundedCornerQuadSdf(highp vec2 uv, highp vec2 size, highp float radius) {
    // Transform to local space where the unit is meters and the origin is at the center
    // of the quad. The abs() call mirrors everything to the positive xy-axes, simplifying
    // calculations since the math is identical in all 4 corners.
    vec2 pointLocal = abs(uv - vec2(0.5)) * size;

    // Shift to a circle space where the origin coincides with the center of the circle
    // that defines the round corner effect.
    highp vec2 pointCircle = pointLocal - (0.5 * size - vec2(radius));

    // To compute the SDF, consider the rounded corner rect as 2 mutually exclusive cases (where @
    // represents the origin of our circle):
    //
    // +---------------+xxxxx
    // |               |    xxxxx
    // |               |         xxx
    // |               |           xxx
    // |   Case 2y     |   Case 2r   xx
    // |               |               x
    // |               |                x
    // |               |                xx
    // |               |                 x
    // |               |                 xx
    // |               @                  x
    // +--------------@@@-----------------+
    // |               @                  |
    // |               |                  |
    // |   Case 1      |   Case 2x        |
    // |               |                  |
    // |               |                  |
    // |               |                  |
    // +---------------+------------------+
    //
    // 1. Case 1: The rectangle spanned by the 4 circle origins (if we hadn't used abs()). This
    //    region can be thought of as calculating the SDF of an "inner" non-rounded quad. This
    //    requires calculating the distance to the closest boundary along either the X/Y axes. This
    //    case only occurs when both components of pointCircle are negative.
    //
    // 2. Case 2: The "outer" regions of our quad. This has 3 cases within it:
    //      Case 2r: In this case, the SDF is simply the L2 norm of the circle space position minus
    //      the radius (giving zero at the boundary). Note that we subtract the radius in all step
    //      so this step is pulled out to the last line.
    //
    //      Case 2x/y: The portions unaffected by the rounded corner. These cases only occur when
    //      only one component of pointCircle is positive. We still want a proper SDF at the
    //      boundary and we get this by projecting the circle space point onto the positive xy-axes
    //      in circle space. This is achieved using max(pointCircle, 0).

    // Case 1
    float innerDistance = min(max(pointCircle.x, pointCircle.y), 0.0);

    // Cases 2r/2x/2y:
    float outerDistance = length(max(pointCircle, 0.0));

    return innerDistance + outerDistance - radius;
}

float circleSDF(highp vec2 uv, vec2 center, float radius) {
    return length(uv - center) - radius;
}
