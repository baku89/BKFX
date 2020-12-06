#version 400

#define SCALE 1024

uniform sampler2D tex0;
uniform float multiplier16bit;
uniform float width;

in vec2 uv;
out vec4 fragColor;

vec4 toAE(vec4 color) {
    return color.argb / multiplier16bit;
}

void main() {
    float distSquared = texture(tex0, uv).r * SCALE;
    float dist = sqrt(distSquared);
    vec4 color = vec4(vec3(dist / width), 1.0);
    fragColor = toAE(color);
}
