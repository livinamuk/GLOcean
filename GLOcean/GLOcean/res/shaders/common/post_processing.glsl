vec3 Tonemap_ACES(const vec3 x) { // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

vec3 AdjustSaturation(vec3 color, float amount) {
    float gray = dot(color, vec3(0.299, 0.587, 0.114)); // Luminance formula
    return mix(vec3(gray), color, 1.0 + amount);
}

vec3 Saturate(vec3 rgb, float adjustment) {
    const vec3 W = vec3(0.2125, 0.7154, 0.0721);
    vec3 intensity = vec3(dot(rgb, W));
    return mix(intensity, rgb, adjustment);
}

vec3 AdjustHue(vec3 color, float hueShift) {
    const mat3 toYIQ = mat3(
        0.299,  0.587,  0.114,
        0.595, -0.274, -0.321,
        0.211, -0.523,  0.311
    );
    const mat3 toRGB = mat3(
        1.0,  0.956,  0.621,
        1.0, -0.272, -0.647,
        1.0, -1.106,  1.703
    );

    vec3 yiq = toYIQ * color;
    float angle = radians(hueShift);
    float cosA = cos(angle);
    float sinA = sin(angle);

    mat3 hueRotation = mat3(
        1.0,      0.0,     0.0,
        0.0,   cosA,   -sinA,
        0.0,   sinA,    cosA
    );

    yiq = hueRotation * yiq;
    return toRGB * yiq;
}

vec3 AdjustLightness(vec3 color, float lightness) {
    float factor = 1.0 + lightness;
    return clamp(color * factor, 0.0, 1.0);
}

vec3 gammaCorrect(vec3 color) {
    return pow(color, vec3(1.0/2.2));
}