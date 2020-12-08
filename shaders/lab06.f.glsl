#version 410 core

// TODO #E add a uniform for our texture map
uniform sampler2D textureMap;

// TODO #D add a varying input to receive the texture coordinate from the vertex shader
in vec2 texCoord;

out vec4 fragColorOut;

void main() {
    // TODO #F perform the texture look up and retrieve the corresponding texel
    vec4 texel = texture(textureMap, texCoord);

    // TODO #G set our fragment color to be the texel color
    fragColorOut = texel;
}