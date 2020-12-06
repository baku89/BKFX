#version 400

#define SCALE 1024

uniform sampler2D tex0;
uniform float beta;
uniform vec2 offset;

in vec2 uv;
out vec4 fragColor;

void main() {
    // https://prideout.net/blog/distance_fields/distance.txt

    vec2 scaledBeta = vec2(beta / SCALE);

    vec2 A = texture(tex0, uv).rg;
    vec2 e = scaledBeta + texture(tex0, uv + offset).rg;
    vec2 w = scaledBeta + texture(tex0, uv - offset).rg;
    vec2 B = min(min(A, e), w);

    // If there is no change, discard the pixel.
    // Convergence can be detected using GL_ANY_SAMPLES_PASSED.
    // if (A == B) {
    //     discard;
    // }

    fragColor = vec4(B, 0.0, 1.0);
}
