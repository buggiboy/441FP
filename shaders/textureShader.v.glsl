#version 410 core

uniform mat4 mvpMtx;

layout(location = 0) in vec3 vPos;
layout(location = 2) in vec2 vTexCoord;

layout(location = 2) out vec2 texCoord;

void main() {
    gl_Position = mvpMtx * vec4(vPos, 1.0);
    texCoord = vTexCoord;
}
