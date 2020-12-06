#version 410 core

uniform sampler2D fbo;

layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec4 fragColorOut;

void main() {
    vec4 texel = texture( fbo, texCoord );
    
    // TODO #A
    float sum = texel.r + texel.g + texel.b;
    sum = sum / 3.0;
    vec4 grayscale = vec4(sum, sum, sum, 1.0);
    fragColorOut = clamp(grayscale, 0.0, 1.0);
}
