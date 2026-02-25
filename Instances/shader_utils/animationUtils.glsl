highp vec2 Flipbook(highp vec2 UV, highp float Width, highp float Height, highp vec2 Direction, highp float SpriteAnimationSpeed, highp float deltaTime) {
    float Square = Width * Height;
    float tile = floor(deltaTime * SpriteAnimationSpeed);
    vec2 TileCount = vec2(1.0, 1.0) / vec2(Width, Height);

    float sequence = mod(tile + float(0.00001), Square) + Square;
    tile = floor((Square - 1.0) * Direction.x + mod(sequence, Square) * Direction.y);

    float base = floor((tile + float(0.5)) * TileCount.x);
    float tileX = (tile - Width * base);
    float tileY = base;

    return (UV + vec2(tileX, tileY)) * TileCount;
}

// Method overload for handling texture with empty frames
highp vec2 Flipbook(highp vec2 uv, vec2 size, highp float progress, float maxFrames){

    progress = floor( mod(progress, maxFrames));
    highp vec2 frame_size = vec2(1.0, 1.0) / vec2(size.x, size.y);
    highp vec2 frame = fract(uv / size) + frame_size;

    frame.x += ( (progress / size.x) - frame_size.x * floor(progress / size.x) * size.x ) - frame_size.x;
    frame.y += (frame_size.y * floor(progress / size.x) ) - frame_size.y;

    return frame;
}