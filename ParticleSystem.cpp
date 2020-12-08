//
// Created by mike.hessel.3 on 11/23/2020.
//


#include "ParticleSystem.h"


// helper functions

// compute and send transformation matrices from main
void particleComputeAndSendTransformationMatrices(glm::mat4 modelMatrix, glm::mat4 viewMatrix, glm::mat4 projectionMatrix,
                                          GLint mvMtxLocation, GLint projMtxLocation) {
    glm::mat4 mvMatrix = viewMatrix * modelMatrix;

    glUniformMatrix4fv(mvMtxLocation, 1, GL_FALSE, &mvMatrix[0][0]);
    glUniformMatrix4fv(projMtxLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
}


ParticleSystem::ParticleSystem() {};

// initiallizes particle vectors for black hole
void ParticleSystem::initialize(glm::vec3 startLoc, float radius) {
    ParticleSystem::_particles = vector<Particle>();

    // initalize the important variables
    _velocityRange = glm::vec2(.005, .05);
    _radius = radius;
    _pos = startLoc;
    _maxLifespan = 20;
    _spawnRate = 20;
    numParticles = 50;     // have a max of 50 particles on the screen
    // setup flat shader
    glm::vec3 flatColor(1.0f, 1.0f, 1.0f);
    _flatShaderProgram->useProgram();
    glUniform3fv(_flatShaderUniforms.color, 1, &flatColor[0]);

    SetUpBuffers();
}

// set up shader attributes
void ParticleSystem::setParticleShaderUandA(CSCI441::ShaderProgram &lightingShader, ParticleShaderUniforms &lightingShaderUniforms,
                                            ParticleShaderAttributes &lightingShaderAttributes) {
    _particleShaderUniforms = lightingShaderUniforms;
    _particleShaderAttributes = lightingShaderAttributes;
    _particleShaderProgram = &lightingShader;
    // sets up textures as well
    particleTextureHandle = CSCI441::TextureUtils::loadAndRegisterTexture("assets/textures/Whoosh.png");
}

// set up shader attributes
void ParticleSystem::setFlatShaderUandA(CSCI441::ShaderProgram &lightingShader, FlatShaderProgramUniforms &lightingShaderUniforms,
                                            FlatShaderProgramAttributes &lightingShaderAttributes) {
    _flatShaderUniforms = lightingShaderUniforms;
    _flatShaderAttributes = lightingShaderAttributes;
    _flatShaderProgram = &lightingShader;
    // sets up textures as well
}

void ParticleSystem::setCameraVariables(glm::vec3 lookAtPoint, glm::vec3 eyePos) {
    _particleShaderUniforms.lookAtPoint = lookAtPoint;
    _particleShaderUniforms.eyePos = eyePos;
}


// update function updates every particle
void ParticleSystem::update(int timePassed, int timeThroughSecond, glm::vec3 position) {

    //update position
    _pos = position;

    vector<int> deadParticles;
    // update particles and find particles that should die
    for(int i = 0; i < _particles.size(); i++) {
        _particles[i].updatePosition();
        if(_particles[i].getLifespan() >= _maxLifespan) {
            deadParticles.push_back(i);
        }
    }

    // remove dead particles
    for(int n = deadParticles.size()-1; n >= 0; n--) {
        _particles.erase(_particles.begin() + deadParticles.at(n));
    }

    // make new particles
    int amount = 1000/_spawnRate;
    int fn = 0;
    if(timePassed > amount) {
        fn = timePassed/amount;
    } else {
        if(std::floor(timeThroughSecond/amount) > std::floor((timeThroughSecond-timePassed)/amount))
            fn = 1;
    }
    for(int n = 0; n < fn; n++) {
        glm::vec3 position;
        glm::vec3 velocity;
        float theta = glm::radians((rand() / (GLfloat)RAND_MAX * 360));
        float phi = glm::radians((rand() / (GLfloat)RAND_MAX * 360));
        float velocityScaler = ((rand() / (GLfloat)RAND_MAX * (_velocityRange.y - _velocityRange.x)) + _velocityRange.x);
        velocity = glm::vec3(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi));
        position = _pos + _radius * velocity;
        velocity = velocity * velocityScaler;

        Particle particle = Particle(position, velocity, 0);
        _particles.push_back(particle);
        //fprintf(stdout, "\nfountain amount: %i", fn);
    }

}


void ParticleSystem::drawBoundings(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, glm::mat4 modelMatrix) {
    _flatShaderProgram->useProgram();

    glm::mat4 mvpMatrix = projectionMatrix * viewMatrix * modelMatrix;
    glUniformMatrix4fv(_flatShaderUniforms.mvpMatrix, 1, GL_FALSE, &mvpMatrix[0][0]);


//    for(int i = 0; i < _clouds.size(); i++) {
//        // draw the points of the wind bounding box
//        int windPoints = 8;
//        glm::vec3 windPositions[windPoints];
//        GLushort windIndices[] = {0,1,2,3,4,5,6,7};
//
//        windPositions[0] = _clouds[i].lowerCorner;
//        windPositions[1] = glm::vec3(_clouds[i].lowerCorner.x,_clouds[i].lowerCorner.y,_clouds[i].upperCorner.z);
//        windPositions[2] = glm::vec3(_clouds[i].upperCorner.x,_clouds[i].lowerCorner.y,_clouds[i].lowerCorner.z);
//        windPositions[3] = glm::vec3(_clouds[i].lowerCorner.x,_clouds[i].upperCorner.y,_clouds[i].lowerCorner.z);
//        windPositions[4] = glm::vec3(_clouds[i].upperCorner.x,_clouds[i].lowerCorner.y,_clouds[i].upperCorner.z);
//        windPositions[5] = glm::vec3(_clouds[i].upperCorner.x,_clouds[i].upperCorner.y,_clouds[i].lowerCorner.z);
//        windPositions[6] = glm::vec3(_clouds[i].lowerCorner.x,_clouds[i].upperCorner.y,_clouds[i].upperCorner.z);
//        windPositions[7] = _clouds[i].upperCorner;
//
//        glBindVertexArray( vaos[VAOS.WIND_BOUNDING] );
//
//        glBindBuffer( GL_ARRAY_BUFFER, vbos[VAOS.WIND_BOUNDING] );
//        glBufferData(GL_ARRAY_BUFFER, windPoints * sizeof(glm::vec3), windPositions, GL_STATIC_DRAW );
//
//        glEnableVertexAttribArray( _flatShaderAttributes.vPos );
//        glVertexAttribPointer( _flatShaderAttributes.vPos, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0 );
//
//        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibos[VAOS.WIND_BOUNDING] );
//        glBufferData(GL_ELEMENT_ARRAY_BUFFER, windPoints * sizeof(GLushort), windIndices, GL_STATIC_DRAW );
//
//        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibos[VAOS.WIND_BOUNDING] );
//        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(GLushort) * windPoints, windIndices);
//
//        glDrawElements(GL_POINTS, windPoints, GL_UNSIGNED_SHORT, (void*)0 );
//    }
}

void ParticleSystem::draw(glm::mat4 viewMatrix, glm::mat4 projectionMatrix) {
    // go through each system vector and draw them with the appropriate shader
    _particleShaderProgram->useProgram();

    // bind and draw water stuff

    // bind particles to the buffer
    int particleCounter = 0;
    for(int n = 0; n < _particles.size(); n++) {
        if(n >= 200) break;
        particleLocations[particleCounter] = _particles[n].getPosition();
        particleType[particleCounter] = _particles[n].getLifespan();
        particleIndices[particleCounter] = particleCounter;
        particleCounter++;
    }


    glBindVertexArray( vaos[VAOS.PARTICLE_SYSTEM] );

    glBindBuffer( GL_ARRAY_BUFFER, vbos[VAOS.PARTICLE_SYSTEM] );
    glBufferData( GL_ARRAY_BUFFER, particleCounter * sizeof(glm::vec3), particleLocations, GL_STATIC_DRAW );

    glEnableVertexAttribArray(_particleShaderAttributes.vPos );
    glVertexAttribPointer(_particleShaderAttributes.vPos, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0 );

    // draw particles
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    particleComputeAndSendTransformationMatrices(modelMatrix, viewMatrix, projectionMatrix,
                                                 _particleShaderUniforms.mvMatrix, _particleShaderUniforms.projMatrix);
    glBindVertexArray( vaos[VAOS.PARTICLE_SYSTEM] );
    glBindTexture(GL_TEXTURE_2D, particleTextureHandle);


    glm::vec3 lookAtPoint = _particleShaderUniforms.lookAtPoint;
    glm::vec3 eyePos = _particleShaderUniforms.eyePos;
    // TODO #1
    glm::vec3 v = normalize(_particleShaderUniforms.lookAtPoint - _particleShaderUniforms.eyePos);    //view vector

    for(int i = 0; i < particleCounter; i++) {
        glm::vec3 currentSprite = particleLocations[particleIndices[i]];    //sprite position
        glm::vec4 p = modelMatrix * glm::vec4(currentSprite, 1);    //sprite point
        glm::vec4 ep = p - glm::vec4(_particleShaderUniforms.eyePos, 1);         //ep vector
        float d = glm::dot(glm::vec4(v,0),ep);
        distances[i] = d;              //distance vector
    }

    // TODO #2
    // sort the indices by distance
    for(int i = 0; i < particleCounter; i++) {
        for(int j = 1; j < particleCounter; j++) {
            float d1 =  distances[j-i];
            float d2 =  distances[j];
            if(distances[j-1] < distances[j]) {
                float temp = distances[j-1];
                distances[j-1] = distances[j];
                distances[j] = temp;
                temp = particleIndices[j-1];
                particleIndices[j-1] = particleIndices[j];
                particleIndices[j] = temp;
            }
        }
    }

    for(int i = 1; i < particleCounter; i++) {
        if(distances[i-1] < distances[i])
            printf("uh oh\n");
    }


    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibos[VAOS.PARTICLE_SYSTEM] );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, particleCounter * sizeof(GLushort), particleIndices, GL_STATIC_DRAW );
    // TODO #3
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibos[VAOS.PARTICLE_SYSTEM] );
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(GLushort) * particleCounter, particleIndices);

    glDrawElements( GL_POINTS, particleCounter, GL_UNSIGNED_SHORT, (void*)0 );

}


void ParticleSystem::SetUpBuffers() {
    // generate ALL VAOs, VBOs, IBOs at once
    glGenVertexArrays( NUM_VAOS, vaos );
    glGenBuffers( NUM_VAOS, vbos );
    glGenBuffers( NUM_VAOS, ibos );

//     --------------------------------------------------------------------------------------------------
//     LOOKHERE #2 - generate sprites

    particleLocations = (glm::vec3*)malloc(sizeof(glm::vec3) * numParticles);
    particleType = (GLuint*)malloc(sizeof(GLushort) * numParticles);
    particleIndices = (GLushort*)malloc(sizeof(GLushort) * numParticles);
    distances = (GLfloat*)malloc(sizeof(GLfloat) * numParticles);

    //fprintf(stdout, "num particles: %i", numParticles);

    glBindVertexArray( vaos[VAOS.PARTICLE_SYSTEM] );

    glBindBuffer( GL_ARRAY_BUFFER, vbos[VAOS.PARTICLE_SYSTEM] );
    glBufferData( GL_ARRAY_BUFFER, numParticles * sizeof(glm::vec3), particleLocations, GL_STATIC_DRAW );

    glEnableVertexAttribArray(_particleShaderAttributes.vPos );
    glVertexAttribPointer(_particleShaderAttributes.vPos, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0 );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibos[VAOS.PARTICLE_SYSTEM] );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, numParticles * sizeof(GLushort), particleIndices, GL_STATIC_DRAW );

    fprintf( stdout, "[INFO]: point sprites read in with VAO/VBO/IBO %d/%d/%d\n", vaos[VAOS.PARTICLE_SYSTEM], vbos[VAOS.PARTICLE_SYSTEM], ibos[VAOS.PARTICLE_SYSTEM] );




}



void ParticleSystem::cleanup() {
    fprintf( stdout, "[INFO]: ...deleting particle shaders....\n" );

    //delete _flatShaderProgram;
    //delete _particleShaderProgram;

    fprintf( stdout, "[INFO]: ...deleting particle IBOs....\n" );

    glDeleteBuffers( NUM_VAOS, ibos );

    fprintf( stdout, "[INFO]: ...deleting particle VBOs....\n" );

    glDeleteBuffers( NUM_VAOS, vbos );
    CSCI441::deleteObjectVBOs();

    fprintf( stdout, "[INFO]: ...deleting particle VAOs....\n" );

    glDeleteVertexArrays( NUM_VAOS, vaos );
    CSCI441::deleteObjectVAOs();

    free(particleLocations);
    free(particleIndices);
    free(particleType);
    free(distances);

    fprintf( stdout, "[INFO]: ...deleting particle textures\n" );

    glDeleteTextures(1, &particleTextureHandle);
}
