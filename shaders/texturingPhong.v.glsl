#version 410 core

uniform mat4 modelviewMtx;
uniform mat4 viewMtx;
uniform mat4 projectionMtx;
uniform mat4 mvpMtx;
uniform mat4 normalMtx;

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoord;

layout(location = 2) out vec2 texCoord;
layout(location = 3) out vec3 normalVec;
layout(location = 4) out vec3 lightVec;
layout(location = 5) out vec3 halfwayVec;

const vec3 LIGHT_POSITION = vec3( 10.0, 10.0, 10.0 );

void main() {
    gl_Position = mvpMtx * vec4(vPos, 1.0);
    texCoord = vTexCoord;

    vec3 cameraVec = normalize( -(modelviewMtx*vec4(vPos,1.0)).xyz );
    normalVec = normalize( (normalMtx * vec4(vNormal,0.0)).xyz );
    lightVec = normalize( (viewMtx * vec4(LIGHT_POSITION,1.0)).xyz - (modelviewMtx*vec4(vPos,1.0)).xyz );
    halfwayVec = normalize( cameraVec + lightVec );
}
