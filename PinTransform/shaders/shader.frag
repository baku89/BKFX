#version 400

uniform sampler2D tex0;

uniform mat3 xformInv;
uniform vec2 resolution;

in vec2 uv;
out vec4 fragColor;

void main() {
    // https://stackoverflow.com/a/42103766
    vec3 coord = vec3(uv.x * resolution.x,
                      (1.0 - uv.y) * resolution.y, 1.0);
    
    vec3 m = vec3(xformInv[0][2], xformInv[1][2], xformInv[2][2]) * coord;
    float zed = 1.0 / (m.x + m.y + m.z);
    coord *= zed;

    float x = xformInv[0][0] * coord.x + xformInv[1][0] * coord.y + xformInv[2][0] * coord.z;
    float y = xformInv[0][1] * coord.x + xformInv[1][1] * coord.y + xformInv[2][1] * coord.z;

    // Normalize back to texture space
    vec2 newUv = vec2(x / resolution.x, 1.0 - y / resolution.y);
    
    fragColor = texture(tex0, newUv);

    // Apply a mask
    vec2 xyMask = step(vec2(0.0), newUv) * step(newUv, vec2(1.0));
    fragColor.r *= xyMask.x * xyMask.y; // R = Alpha in ARGB
}
