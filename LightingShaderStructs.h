//
// Created by mike.hessel.3 on 11/23/2020.
//

#ifndef LAB10_LIGHTINGSHADERSTRUCTS_H
#define LAB10_LIGHTINGSHADERSTRUCTS_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>			// include GLFW framework header

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct ParticleShaderUniforms {
    GLint mvMatrix;                     // the ModelView Matrix to apply
    GLint projMatrix;                   // the Projection Matrix to apply
    GLint image;                        // the texture to bind
    glm::vec3 eyePos;                   // camera position
    glm::vec3 lookAtPoint;              // location of our object of interest to view
};

struct ParticleShaderAttributes {
    GLint vPos;                         // the vertex position
    GLint lifespan;
};


struct FlatShaderProgramUniforms {
    GLint mvpMatrix;                    // the MVP Matrix to apply
    GLint color;                        // the color to apply
};
struct FlatShaderProgramAttributes {
    GLint vPos;                         // the vertex position
};

#endif //LAB10_LIGHTINGSHADERSTRUCTS_H
