#version 410 core

uniform vec4 materialDiffuse;
uniform vec4 materialSpecular;
uniform float materialShininess;
uniform vec4 materialAmbient;
uniform sampler2D txtr;

layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 normalVec;
layout(location = 4) in vec3 lightVec;
layout(location = 5) in vec3 halfwayVec;

layout(location = 0) out vec4 fragColorOut;

const vec4 LIGHT_DIFFUSE = vec4(1.0, 1.0, 1.0, 1.0);
const vec4 LIGHT_SPECULAR = vec4(1.0, 1.0, 1.0, 1.0);
const vec4 LIGHT_AMBIENT = vec4(1.0, 1.0, 1.0, 1.0);

void main() {
    vec3 lightVec2 = normalize(lightVec);
    vec3 normalVec2 = normalize(normalVec);
    vec3 halfwayVec2 = normalize(halfwayVec);
    
    float sDotN = max( dot(lightVec2, normalVec2), 0.0 );
    vec4 diffuse = LIGHT_DIFFUSE * materialDiffuse * sDotN;
    
    vec4 specular = vec4(0.0);
    if( sDotN > 0.0 )
        specular = LIGHT_SPECULAR * materialSpecular * pow( max( 0.0, dot( halfwayVec2, normalVec2 ) ), materialShininess );
    
    vec4 ambient = LIGHT_AMBIENT * materialAmbient;
    
    fragColorOut = diffuse + specular + ambient;
    
    vec4 texel = texture( txtr, texCoord );
    fragColorOut *= texel;
}
