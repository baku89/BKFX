#version 400

uniform sampler2D tex0;
uniform float multiplier16bit;
uniform float infinity;
uniform int source;

#define SOURCE_LUMA 1

in vec2 uv;
out vec4 fragColor;

vec4 fromAE(vec4 color) {
    return color.gbar * multiplier16bit;
}

void main() {
    vec4 color = fromAE(texture(tex0, uv));

    float value = color.a;

    if (source == SOURCE_LUMA) {
      value = dot(color.rgb, vec3(1.0 / 3.0));
    }

    float outside = step(color.r, 0.5);
    vec2 mask = vec2(outside, 1.0 - outside);
    
    fragColor = vec4(mask * infinity, 0.0, 1.0);
}
