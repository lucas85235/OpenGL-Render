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