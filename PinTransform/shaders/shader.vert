#version 400

in vec2 aPos;
out vec2 uv;

uniform vec2 resolution;
uniform mat3 xform;

void main() {
    vec2 coord = vec2(aPos.x * resolution.x,
                      (1.0 - aPos.y) * resolution.y);
    vec2 newCoord = (xform * vec3(coord, 1.0)).xy;
    vec2 pos = vec2(newCoord.x / resolution.x,
                    1.0 - newCoord.y / resolution.y);
    gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);
    uv = aPos;
}
