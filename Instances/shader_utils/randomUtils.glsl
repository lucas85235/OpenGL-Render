highp float hash21(highp vec2 n, highp float index, highp float amount) {
    index = index/amount;
    float phase = index * 2.0 * 3.1415;
    return (sin(n.x * n.y * phase) + 1.0) / 2.0;
}
