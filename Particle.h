//
// Created by mike.hessel.3 on 11/23/2020.
//

#ifndef LAB10_PARTICLE_H
#define LAB10_PARTICLE_H

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
#include <string>

class Particle {
public:
    // type - (0-fountain,1-rain, 2-splash, 3-butterfly)
    Particle(glm::vec3 position, glm::vec3 velocity, int type);

    int getLifespan();
    glm::vec3 getPosition();
    glm::vec3 getVelocity();
    int getType();

    // move once along velocity
    void updatePosition();

    // set new position
    void setPosition(glm::vec3 pos);

    // add the new and old velocity
    void updateVelocity(glm::vec3 newVelocity);

    // set the old velocity to the new velocity
    void setVelocity(glm::vec3 newVelocity);

private:
    glm::vec3 position;       // position
    glm::vec3 velocity;    // velocity
    int type;
    int lifespan;
};


#endif //LAB10_PARTICLE_H
