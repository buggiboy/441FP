#version 410 core

uniform mat4 mvpMatrix;

layout(location = 0) in vec3 vPos;
// TODO #A add an attribute for the texture coordinate
in vec2 texCoordIn;

// TODO #B add a varying output to send the texture coordinate to the fragment shader
out vec2 texCoord;

void main() {
    gl_Position = mvpMatrix * vec4(vPos, 1.0);

    // TODO #C set the varying to our attribute
    texCoord = texCoordIn;
}