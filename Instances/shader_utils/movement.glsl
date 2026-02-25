vec3 MRU (vec3 initalPosition, vec3 velocity, float iTime) {
    return initalPosition + velocity * iTime;
}

float MRU (float initalPosition, float velocity, float iTime) {
    return initalPosition + velocity * iTime;
}

vec3 MHS (vec3 initalPosition, vec3 A, float w, float phase, float iTime) {
    return initalPosition + A * cos(w*iTime + phase);
}

vec3 MUV (vec3 initalPosition, vec3 initialVelocity, vec3 aceleration, float iTime) {
    return initalPosition + initialVelocity * iTime + 0.5 * aceleration * pow(iTime,2.0);
}

vec2 MCU (vec2 initialPosition, float angularVelocity, float radius, float iTime) {
    float X = initialPosition[0] + radius * cos(angularVelocity * iTime);
    float Y = initialPosition[1] + radius * sin(angularVelocity * iTime);
    return vec2(X,Y);
}

vec2 MCA(vec2 initialPosition, float angularVelocity, float angularAceleration, float radius, float iTIme) {
    float theta = angularVelocity * iTIme + 0.5 * angularAceleration * pow(iTIme, 2.0);
    float X = initialPosition[0] + radius * cos(theta);
    float Y = initialPosition[1] + radius * sin(theta);

    return vec2(X,Y);
}

vec4 RotationXYZ(vec4 pos, vec3 velocity, float iTime) {

    vec4 vertexPosition = pos;
    vec3 angle = velocity * iTime;

    float cx = cos(angle.x);
    float cy = cos(angle.y);
    float cz = cos(angle.z);
    float sx = sin(angle.x);
    float sy = sin(angle.y);
    float sz = sin(angle.z);

    vertexPosition *= mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, cx,  sx,  0.0,
        0.0, -sx, cx,  0.0,
        0.0, 0.0, 0.0, 1.0
    );

    vertexPosition *= mat4(
        cy,  0.0, sy,  0.0,
        0.0, 1.0, 0.0, 0.0,
        -sy, 0.0, cy,  0.0,
        0.0, 0.0, 0.0, 1.0
    );

    vertexPosition *= mat4(
        cz,  sz,  0.0, 0.0,
        -sz, cz,  0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    );

    return vertexPosition;
}

vec3 MP(vec3 position, vec3 velocity, float aceleration, float iTime) {
    float x = position.x + velocity.x * iTime;
    float z = position.z + velocity.z * iTime;
    float y = position.y + velocity.y * iTime - 0.5 * aceleration * pow(iTime, 2.0);

    return vec3(x,y,z);
}

vec3 SNOW(vec3 position, float iTime, vec3 A, vec3 P, vec3 v) {
    vec3 pos = position;
    pos.x += v.x * iTime + A.x * sin(P.x * iTime);
    pos.y += v.y * iTime + A.y * sin(P.y * iTime);
    pos.z += v.z * iTime + A.z * sin(P.z * iTime);

    return pos;
}

vec3 CONE(vec3 position, vec3 velocity, float w, float initial_radius, float final_radius, float life, float iTime)  {
    float t = float(iTime / life);
    vec3 pos = position;
    float radius = mix(initial_radius, final_radius, t);
    pos.xz = MCU(pos.xz, w, radius, iTime);
    pos = MRU(pos, velocity, iTime);

    return pos;
}


