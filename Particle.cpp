//
// Created by mike.hessel.3 on 11/23/2020.
//

#include "Particle.h"

// type - (0-fountain,1-rain,2-butterfly)
Particle::Particle(glm::vec3 position, glm::vec3 velocity, int type) {
    this->position = position;
    this->velocity = velocity;
    this->type = type;
    lifespan = 0;
}


int Particle::getLifespan() {
    return lifespan;
}


glm::vec3 Particle::getPosition() {
    return position;
}

glm::vec3 Particle::getVelocity() {
    return velocity;
}

int Particle::getType() {
    return type;
}


// move once along velocity and update lifespan
void Particle::updatePosition() {
    position = position + velocity;
    lifespan++;
}

// set new position
void Particle::setPosition(glm::vec3 pos) {
    position = pos;
}

// add the new and old velocity
void Particle::updateVelocity(glm::vec3 newVelocity) {
    velocity = velocity + newVelocity;
}

// set the old velocity to the new velocity
void Particle::setVelocity(glm::vec3 newVelocity) {
    velocity = newVelocity;
}