//
// Created by mike.hessel.3 on 11/23/2020.
//

#ifndef LAB10_PARTICLESYSTEM_H
#define LAB10_PARTICLESYSTEM_H


// opengl and glm libraries
#include <GL/glew.h>                    // define our OpenGL extensions
#include <GLFW/glfw3.h>			        // include GLFW framework header

#include <glm/glm.hpp>                  // include GLM libraries
#include <glm/gtc/matrix_transform.hpp> // and matrix functions

// include C and C++ libraries
#include <cmath>				// for cos(), sin() functionality
#include <cstdio>				// for printf functionality
#include <cstdlib>			    // for exit functionality
#include <ctime>			    // for time() functionality
#include <vector>
#include <iostream>
#include <sstream>
#include <string>

// class libraries
#include <CSCI441/OpenGLUtils.hpp>      // prints OpenGL information
#include <CSCI441/objects.hpp>          // draws 3D objects
#include <CSCI441/ShaderProgram.hpp>    // wrapper class for GLSL shader programs
#include <CSCI441/TextureUtils.hpp>     // convenience for loading textures

// other classes
#include "Particle.h"
#include "LightingShaderStructs.h"

class ParticleSystem {
public:

    ParticleSystem();
    void initialize(glm::vec3 startLoc, float radius);
    void setParticleShaderUandA(CSCI441::ShaderProgram &lightingShader, ParticleShaderUniforms &lightingShaderUniforms,
                                ParticleShaderAttributes &lightingShaderAttributes);
    void setFlatShaderUandA(CSCI441::ShaderProgram &lightingShader, FlatShaderProgramUniforms &lightingShaderUniforms,
                                FlatShaderProgramAttributes &lightingShaderAttributes);
    void setCameraVariables(glm::vec3 lookAtPoint, glm::vec3 eyePos);

    void update(int timePassed, int timeThroughSecond, glm::vec3 position);  // takes in the time passed in milliseconds

    void draw(glm::mat4 viewMatrix, glm::mat4 projectionMatrix);
    void drawBoundings(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, glm::mat4 modelMatrix);   // draw the different bounding boxes.

    void cleanup();

private:

    void SetUpBuffers();

    // shader stuff (I'll figure that out tomorrow)
    CSCI441::ShaderProgram *_particleShaderProgram = nullptr;
    ParticleShaderUniforms _particleShaderUniforms;
    ParticleShaderAttributes _particleShaderAttributes;
    CSCI441::ShaderProgram *_flatShaderProgram = nullptr;
    FlatShaderProgramAttributes _flatShaderAttributes;
    FlatShaderProgramUniforms _flatShaderUniforms;

// all drawing information
    const struct VAO_IDS {
        GLuint PARTICLE_SYSTEM = 0;
    } VAOS;
    const static GLuint NUM_VAOS = 1;
    GLuint vaos[NUM_VAOS];                  // an array of our VAO descriptors
    GLuint vbos[NUM_VAOS];                  // an array of our VBO descriptors
    GLuint ibos[NUM_VAOS];                  // an array of our IBO descriptors
    GLuint particleTextureHandle;             // the texture to apply to the particle (all water)
    GLuint numParticles = 0;                // the number of particles on the screen
    glm::vec3* particleLocations = nullptr;   // the (x,y,z) location of each particle
    GLuint* particleType = nullptr;           // the type of the particle
    GLushort* particleIndices = nullptr;      // the order to draw the particles in
    GLfloat* distances = nullptr;           // will be used to store the distance to the camera

    // particle information
    vector<Particle> _particles;
    glm::vec3 _pos;
    float _radius;
    glm::vec2 _velocityRange;
    GLint _maxLifespan;
    int _spawnRate;

    const glm::vec3 GRAVITY = glm::vec3(0,-.056,0);
};

#endif //LAB10_PARTICLESYSTEM_H
