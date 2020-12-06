#version 400

#define SCALE 1024

uniform sampler2D tex0;
uniform float multiplier16bit;
uniform float width;

uniform int mode;
#define MODE_INSIDE         1
#define MODE_OUTSIDE        2
#define MODE_BOTH_SIGNED    3
#define MODE_BOTH_ABS       4

uniform int invert;

in vec2 uv;
out vec4 fragColor;

vec4 toAE(vec4 color) {
    return color.argb / multiplier16bit;
}

void main() {
    vec2 distSquared = texture(tex0, uv).rg * SCALE;
    vec2 dist = sqrt(distSquared);
    vec2 normDist = dist / width;

    float luma = 0.0;

    if (mode == MODE_INSIDE) {
        luma = normDist.g;
    } else if (mode == MODE_OUTSIDE) {
        luma = normDist.r;
    } else if (mode == MODE_BOTH_SIGNED) {
        luma = 0.5 + (normDist.r - normDist.g) / 2.0;
    } else {
        luma = abs(normDist.r + normDist.g);
    }

    vec4 color = vec4(vec3(invert == 1 ? 1.0 - luma : luma), 1.0);
    fragColor = toAE(color);
}
