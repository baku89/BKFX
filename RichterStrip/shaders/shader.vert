attribute vec2 aPos;
varying vec2 uv;

void main() {
    gl_Position = vec4(aPos * 2.0 - 1.0, 0.0, 1.0);
    uv = aPos;
}
