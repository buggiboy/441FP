/*
 *   Vertex Shader
 *
 *   CSCI 441, Computer Graphics, Colorado School of Mines
 */

#version 410 core

in vec3 vPos;

uniform mat4 mvMatrix;

void main() {
    /*****************************************/
    /********* Vertex Calculations  **********/
    /*****************************************/
    gl_Position = mvMatrix * vec4(vPos, 1.0);
}
