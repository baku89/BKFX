#version 400

uniform sampler2D tex0;
uniform float multiplier16bit;

in vec2 uv;
out vec4 fragColor;

vec4 fromAE(vec4 color) {
    return color.gbar * multiplier16bit;
}

vec4 toAE(vec4 color) {
    return color.argb / multiplier16bit;
}

void main() {
    fragColor = texture(tex0, uv);
}
