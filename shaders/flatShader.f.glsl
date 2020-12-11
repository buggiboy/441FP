#version 410 core

// uniform inputs
uniform vec3 color;

// varying inputs

// outputs
out vec4 fragColorOut;

void main() {
    fragColorOut = vec4(color, 1.0);
}