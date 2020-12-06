#version 410 core

uniform sampler2D tex;

layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec4 fragColorOut;

void main() {
  fragColorOut = texture( tex, texCoord );
}
