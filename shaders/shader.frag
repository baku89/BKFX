#version 400

uniform sampler2D tex0;
uniform float multiplier16bit;

uniform vec2 center;
uniform float angle;
uniform float aspectY;

in vec2 uv;
out vec4 fragColor;

vec4 fromAE(vec4 color) {
    return color.gbar * multiplier16bit;
}

vec4 toAE(vec4 color) {
    return color.argb / multiplier16bit;
}

void main() {
    vec2 p = uv     * vec2(1.0, aspectY);
    vec2 c = center * vec2(1.0, aspectY);
    
    vec2 dir = vec2(cos(angle), sin(angle));
    
    float l = dot(p - c, dir);
    
    vec2 newUV = (c + dir * l) / vec2(1.0, aspectY);
    
    fragColor = texture(tex0, newUV);
}
