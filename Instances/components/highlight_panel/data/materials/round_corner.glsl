// TODO(cburden): Update this function to remove reliance on if statements.
float alphaAtCorner(highp vec2 uv, highp float r, highp float aspect_ratio, highp float alpha) {
    bool isFill = (uv.x > r && uv.x < (1.0 - r)) || (uv.y > r * aspect_ratio && uv.y < (1.0 - r * aspect_ratio));
    if (uv.x <= r && uv.y <= r * aspect_ratio) {   // bottom left
        isFill = pow(r - uv.x, 2.0) + pow(r - uv.y / aspect_ratio, 2.0) <= pow(r, 2.0);
    } else if (uv.x >= (1.0 - r) && uv.y <= r * aspect_ratio) {    // bottom right
        isFill = pow(r - 1.0 + uv.x, 2.0) + pow(r - uv.y / aspect_ratio, 2.0) <= pow(r, 2.0);
    } else if (uv.x <= r && uv.y >= (1.0 - r * aspect_ratio)) {    // top left
        isFill = pow(r - uv.x, 2.0) + pow(r - 1.0 / aspect_ratio + uv.y / aspect_ratio, 2.0) <= pow(r, 2.0);
    } else if (uv.x >= (1.0 - r) && uv.y >= (1.0 - r * aspect_ratio)) {  // top right
        isFill = pow(r - 1.0 + uv.x, 2.0) + pow(r - 1.0 / aspect_ratio + uv.y / aspect_ratio, 2.0) <= pow(r, 2.0);
    }

    if (isFill) {
        return alpha;
    }

    return 0.0;
}