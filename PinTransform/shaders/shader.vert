#version 400

uniform vec2 resolution;
uniform mat3 xform;

in vec2 aPos;
out vec2 uv;

void main() {
    // Convert to pixel coordinate
    vec2 coord = vec2(aPos.x * resolution.x, (1.0 - aPos.y) * resolution.y);
    
    vec3 newCoord3D = xform * vec3(coord, 1.0);
    vec2 newCoord = newCoord3D.xy / newCoord3D.z;
    
    // Convert back to UV space
    vec2 newPos = vec2(newCoord.x / resolution.x, 1.0 - newCoord.y / resolution.y);
    
    // Assign 'em to output
    gl_Position = vec4(newPos * 2.0 - 1.0, 0.0, 1.0);
    uv = newPos;
}
