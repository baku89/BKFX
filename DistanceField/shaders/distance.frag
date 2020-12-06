#version 400

#define SCALE 1024

uniform sampler2D tex0;
uniform float beta;
uniform vec2 offset;

in vec2 uv;
out vec4 fragColor;

void main() {
		// https://prideout.net/blog/distance_fields/distance.txt

    float A = texture(tex0, uv).r;
    float e = beta / SCALE + texture(tex0, uv + offset).r;
    float w = beta / SCALE + texture(tex0, uv - offset).r;

    float B = min(min(A, e), w);

    // If there is no change, discard the pixel.
    // Convergence can be detected using GL_ANY_SAMPLES_PASSED.
    // if (A == B) {
    //     discard;
    // }

    fragColor = vec4(vec3(B), 1.0);
}
