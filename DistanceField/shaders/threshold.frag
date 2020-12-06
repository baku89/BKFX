#version 400

uniform sampler2D tex0;
uniform float multiplier16bit;
uniform float infinity;

in vec2 uv;
out vec4 fragColor;

vec4 fromAE(vec4 color) {
    return color.gbar * multiplier16bit;
}

void main() {
    vec4 color = fromAE(texture(tex0, uv));
    fragColor = vec4(vec3(step(color.r, 0.5)) * infinity, 1.0);
}
