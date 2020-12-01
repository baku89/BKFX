#version 400

in vec2 aPos;
out vec2 uv;

uniform mat3 xform;

void main() {
    vec2 newPos = (xform * vec3(aPos, 1.0)).xy;
    gl_Position = vec4(newPos * 2.0 - 1.0, 0.0, 1.0);
    uv = aPos;
}
