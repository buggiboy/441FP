/*
 *   Geometry Shader
 *
 *   CSCI 441, Computer Graphics, Colorado School of Mines
 */

#version 410 core

// TODO #A
layout( points ) in;

// TODO #B
layout( triangle_strip, max_vertices = 4 ) out;

uniform mat4 projMatrix;

// TODO #I
out vec2 texCoord;

void main() {

    // TODO #C
    gl_Position = projMatrix * (gl_in[0].gl_Position + vec4(-0.2,-0.2,0,0));

    // TODO #D
    texCoord = vec2(0,0);
    EmitVertex();

    // TODO #F
    gl_Position = projMatrix * (gl_in[0].gl_Position + vec4(-0.2,0.2,0,0));
    texCoord = vec2(1,0);
    EmitVertex();

    // TODO #G
    gl_Position = projMatrix * (gl_in[0].gl_Position + vec4(0.2,-0.2,0,0));
    texCoord = vec2(0,1);
    EmitVertex();

    // TODO #H
    gl_Position = projMatrix * (gl_in[0].gl_Position + vec4(0.2,0.2,0,0));
    texCoord = vec2(1,1);
    EmitVertex();

    // TODO #E
    EndPrimitive();
}
