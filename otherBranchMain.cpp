/*
 *  CSCI 441, Computer Graphics, Fall 2020
 *
 *  Project: lab10
 *  File: main.cpp
 *
 *  Description:
 *      This file contains the start to render billboarded sprites.
 *
 *  Author: Dr. Paone, Colorado School of Mines, 2020
 *
 */

//***********************************************************************************************************************************************************
//
// Library includes

#include <GL/glew.h>                    // define our OpenGL extensions
#include <GLFW/glfw3.h>			        // include GLFW framework header
#include <CSCI441/modelLoader.hpp>      // to load in OBJ models

#include <glm/glm.hpp>                  // include GLM libraries
#include <glm/gtc/matrix_transform.hpp> // and matrix functions

#include <cstdio>				        // for printf functionality
#include <cstdlib>				        // for exit functionality
#include <chrono>                       // for high resolution time

#include <CSCI441/materials.hpp>        // our pre-defined material properties
#include <CSCI441/OpenGLUtils.hpp>      // prints OpenGL information
#include <CSCI441/objects.hpp>          // draws 3D objects
#include <CSCI441/ShaderProgram.hpp>    // wrapper class for GLSL shader programs
#include <CSCI441/TextureUtils.hpp>     // convenience for loading textures

#include "LightingShaderStructs.h"
#include "ParticleSystem.h"
#include <glm/gtx/quaternion.hpp>

#include "Transform.h"


#define STB_IMAGE_IMPLEMENTATION
//#include <CSCI441/stb_image.h>
//***********************************************************************************************************************************************************
//
// Global Parameters

// fix our window to a specific size
const GLint WINDOW_WIDTH = 640, WINDOW_HEIGHT = 640;

// keep track our mouse information
GLboolean controlDown;                  // if the control button was pressed when the mouse was pressed
GLboolean leftMouseDown;                // if the mouse left button is pressed
glm::vec2 mousePosition;                // current mouse position

GLuint lightType;                       // type of the light - 0 point 1 directional 2 spot
bool drawBoundings;

// keep track of all our camera information
struct CameraParameters {
    glm::vec3 cameraAngles;             // cameraAngles --> x = theta, y = phi, z = radius
    glm::vec3 camDir;                   // direction to the camera
    glm::vec3 eyePos;                   // camera position
    glm::vec3 lookAtPoint;              // location of our object of interest to view
    glm::vec3 upVector;                 // the upVector of our camera
} arcballCam;

// time information
unsigned long long now;

// all drawing information
const struct VAO_IDS {
    GLuint PARTICLE_SYSTEM = 0;
} VAOS;
const GLuint NUM_VAOS = 1;
GLuint vaos[NUM_VAOS];                  // an array of our VAO descriptors
GLuint vbos[NUM_VAOS];                  // an array of our VBO descriptors
GLuint ibos[NUM_VAOS];                  // an array of our IBO descriptors

// Particle System
ParticleSystem particleSystem;

// point sprite information
const GLuint NUM_SPRITES = 75;          // the number of sprites to draw
const GLfloat MAX_BOX_SIZE = 10;        // our sprites exist within a box of this size
glm::vec3* spriteLocations = nullptr;   // the (x,y,z) location of each sprite
GLushort* spriteIndices = nullptr;      // the order to draw the sprites in
GLfloat* distances = nullptr;           // will be used to store the distance to the camera
GLuint spriteTextureHandle;             // the texture to apply to the sprite
GLfloat snowglobeAngle;                 // rotates all of our snowflakes

CSCI441::ModelLoader* model = nullptr;  // assign as a null pointer to delay creation until
GLint vpos_attrib_location;
struct suckableObject   {
    glm::vec3 color;
    glm::vec3 ambient;
    glm::vec3 spec;
    glm::vec3 position;
    glm::vec3 velocity;

    glm::vec3 rotationalVelocity;
    float shininess = 1;
    //glm::quat rotation;
    Transform transform;

};
suckableObject myTeapot;
suckableObject myCube;
suckableObject myBulb;

// Billboard shader program
CSCI441::ShaderProgram *billboardShaderProgram = nullptr;
struct BillboardShaderProgramUniforms {
    GLint mvMatrix;                     // the ModelView Matrix to apply
    GLint projMatrix;                   // the Projection Matrix to apply
    GLint image;                        // the texture to bind
} billboardShaderProgramUniforms;
struct BillboardShaderProgramAttributes {
    GLint vPos;                         // the vertex position
} billboardShaderProgramAttributes;
ParticleShaderUniforms fountainShaderUniforms;
ParticleShaderAttributes fountainShaderAttributes;

CSCI441::ShaderProgram *flatShaderProgram = nullptr;
FlatShaderProgramUniforms flatShaderProgramUniforms;
FlatShaderProgramAttributes flatShaderProgramAttributes;

// skybox and ground stuff
GLuint platformVAO, platformVBOs[2];    // the ground platform everything is hovering over
GLuint skyboxFrontVAO, skyboxFrontVBOs[2], skyboxSideVAO, skyboxSideVBOs[2], skyboxTopVAO, skyboxTopVBOs[2];
GLuint skyboxSidesTextureHandle;
GLuint skyboxTopTextureHandle;

// gourad with phong illumination shader program
CSCI441::ShaderProgram *gouradShaderProgram = nullptr;
struct GouradShaderProgramUniforms {
    GLint mvpMatrix;                    // the MVP Matrix to apply
    GLint modelMatrix;                  // model matrix
    GLint normalMtx;                    // normal matrix
    GLint eyePos;                       // camera position
    GLint lightPos;                     // light position - used for point/spot
    GLint lightDir;                     // light direction - used for directional/spot
    GLint lightCutoff;                  // light cone angle - used for spot
    GLint lightColor;                   // color of the light
    GLint lightType;                    // type of the light - 0 point 1 directional 2 spot
    GLint materialDiffColor;            // material diffuse color
    GLint materialSpecColor;            // material specular color
    GLint materialShininess;            // material shininess factor
    GLint materialAmbColor;             // material ambient color
} gouradShaderProgramUniforms;
struct GouradShaderProgramAttributes {
    GLint vPos;                         // position of our vertex
    GLint vNormal;                      // normal for the vertex
} gouradShaderProgramAttributes;

// keep track of our texture shader program
CSCI441::ShaderProgram *texShaderProgram = nullptr;
struct TexShaderProgramUniforms {
    GLint mvpMatrix;                    // the MVP Matrix to apply
    // TODO #11 add a uniform location for our texture map
    GLint textureMap;

} texShaderProgramUniforms;
struct TexShaderProgramAttributes {
    GLint vPos;                         // position of our vertex
    // TODO #10 add an attribute location for our texture coordinate
    GLint texCoordIn;

} texShaderProgramAttributes;


//***********************************************************************************************************************************************************
//
// Helper Functions

// updateCameraDirection() /////////////////////////////////////////////////////////////////////////////
/// \desc
/// This function updates the camera's position in cartesian coordinates based
///  on its position in spherical coordinates. Should be called every time
///  cameraAngles is updated.
///
// /////////////////////////////////////////////////////////////////////////////
void updateCameraDirection() {
    // ensure the camera does not flip upside down at either pole
    if( arcballCam.cameraAngles.y < 0 )     arcballCam.cameraAngles.y = 0.0f + 0.001f;
    if( arcballCam.cameraAngles.y >= M_PI ) arcballCam.cameraAngles.y = M_PI - 0.001f;

    // do not let our camera get too close or too far away
    if( arcballCam.cameraAngles.z <= 2.0f )  arcballCam.cameraAngles.z = 2.0f;
    if( arcballCam.cameraAngles.z >= 30.0f ) arcballCam.cameraAngles.z = 30.0f;

    // update the new direction to the camera
    arcballCam.camDir.x =  sinf( arcballCam.cameraAngles.x ) * sinf( arcballCam.cameraAngles.y );
    arcballCam.camDir.y = -cosf( arcballCam.cameraAngles.y );
    arcballCam.camDir.z = -cosf( arcballCam.cameraAngles.x ) * sinf( arcballCam.cameraAngles.y );

    // normalize this direction
    arcballCam.camDir = glm::normalize(arcballCam.camDir);
}

// computeAndSendTransformationMatrices() //////////////////////////////////////////////////////////////////////////////
/// \desc
/// This function sends the matrix uniforms to a given shader location.  Precomputes the ModelView Matrix
/// \param modelMatrix - current Model Matrix
/// \param viewMatrix - current View Matrix
/// \param projectionMatrix - current Projection Matrix
/// \param mvMtxLocation - location within currently bound shader to send the ModelView Matrix
/// \param projMtxLocation - location within currently bound shader to send the Projection Matrix
// //////////////////////////////////////////////////////////////////////////////
void computeAndSendTransformationMatrices(glm::mat4 modelMatrix, glm::mat4 viewMatrix, glm::mat4 projectionMatrix,
                                          GLint mvMtxLocation, GLint projMtxLocation) {
    glm::mat4 mvMatrix = viewMatrix * modelMatrix;

    glUniformMatrix4fv(mvMtxLocation, 1, GL_FALSE, &mvMatrix[0][0]);
    glUniformMatrix4fv(projMtxLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
}

/// computeAndSendTransformationMatrices() //////////////////////////////////////
///
/// This function sends the matrix uniforms to a given shader location.
///
////////////////////////////////////////////////////////////////////////////////
void computeAndSendTransformationMatrices(glm::mat4 modelMatrix, glm::mat4 viewMatrix, glm::mat4 projectionMatrix,
                                          GLint mvpMtxLocation, GLint modelMtxLocation = -1, GLint normalMtxLocation = -1) {
    glm::mat4 mvpMatrix = projectionMatrix * viewMatrix * modelMatrix;
    glm::mat3 normalMatrix = glm::mat3( glm::transpose( glm::inverse(modelMatrix) ) );

    glUniformMatrix4fv(mvpMtxLocation, 1, GL_FALSE, &mvpMatrix[0][0]);
    glUniformMatrix4fv(modelMtxLocation, 1, GL_FALSE, &modelMatrix[0][0]);
    glUniformMatrix3fv(normalMtxLocation, 1, GL_FALSE, &normalMatrix[0][0]);
}


// randNumber() /////////////////////////////////////////////////////////////////////////////
/// \dexc generates a random float between [-max, max]
/// \param max - lower & upper bound to generate value between
/// \return float within range [-max, max]
// //////////////////////////////////////////////////////////////////////////////
GLfloat randNumber( GLfloat max ) {
    return rand() / (GLfloat)RAND_MAX * max * 2.0 - max;
}

//***********************************************************************************************************************************************************
//
// Event Callbacks

// error_callback() /////////////////////////////////////////////////////////////////////////////
/// \desc
///		We will register this function as GLFW's error callback.
///	When an error within GLFW occurs, GLFW will tell us by calling
///	this function.  We can then print this info to the terminal to
///	alert the user.
///
// /////////////////////////////////////////////////////////////////////////////
static void error_callback(int error, const char* description) {
    fprintf(stderr, "[ERROR]: (%d) %s\n", error, description);
}

// key_callback() /////////////////////////////////////////////////////////////////////////////
/// \desc
///		We will register this function as GLFW's keypress callback.
///	Responds to key presses and key releases
///
// /////////////////////////////////////////////////////////////////////////////
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if(action == GLFW_PRESS) {
        switch( key ) {
            case GLFW_KEY_Q:
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose( window, GLU_TRUE );
                break;
            case GLFW_KEY_3:    // spot light
                lightType = key - GLFW_KEY_1;
                // send the light type to the shader
                gouradShaderProgram->useProgram();
                glUniform1i(gouradShaderProgramUniforms.lightType, lightType);
                break;
            case GLFW_KEY_B:
                drawBoundings = !drawBoundings;
                break;
            default: break;
        }
    }
}

// mouse_button_callback() /////////////////////////////////////////////////////////////////////////////
/// \desc
///		We will register this function as GLFW's mouse button callback.
///	Responds to mouse button presses and mouse button releases.  Keeps track if
///	the control key was pressed when a left mouse click occurs to allow
///	zooming of our arcball camera.
///
// /////////////////////////////////////////////////////////////////////////////
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if( button == GLFW_MOUSE_BUTTON_LEFT ) {
        if( action == GLFW_PRESS ) {
            leftMouseDown = GL_TRUE;
            controlDown = (mods & GLFW_MOD_CONTROL);
        } else {
            leftMouseDown = GL_FALSE;
            controlDown = GL_FALSE;
            mousePosition = glm::vec2(-9999.0f, -9999.0f);
        }
    }
}

// cursor_callback() /////////////////////////////////////////////////////////////////////////////
/// \desc
///		We will register this function as GLFW's cursor movement callback.
///	Responds to mouse movement.  When active motion is used with the left
///	mouse button an arcball camera model is followed.
///
// /////////////////////////////////////////////////////////////////////////////
static void cursor_callback( GLFWwindow* window, double xPos, double yPos ) {
    // make sure movement is in bounds of the window
    // glfw captures mouse movement on entire screen
    if( xPos > 0 && xPos < WINDOW_WIDTH ) {
        if( yPos > 0 && yPos < WINDOW_HEIGHT ) {
            // active motion
            if( leftMouseDown ) {
                if( (mousePosition.x - -9999.0f) > 0.001f ) {
                    if( !controlDown ) {
                        // if control is not held down, update our camera angles theta & phi
                        arcballCam.cameraAngles.x += (xPos - mousePosition.x) * 0.005f;
                        arcballCam.cameraAngles.y += (mousePosition.y - yPos) * 0.005f;
                    } else {
                        // otherwise control was held down, update our camera radius
                        double totChgSq = (xPos - mousePosition.x) + (yPos - mousePosition.y);
                        arcballCam.cameraAngles.z += totChgSq*0.01f;
                    }
                    // recompute our camera direction
                    updateCameraDirection();
                }
                // update the last mouse position
                mousePosition = glm::vec2(xPos, yPos);
            }
                // passive motion
            else {

            }
        }
    }
}

// scroll_callback() /////////////////////////////////////////////////////////////////////////////
/// \desc
///		We will register this function as GLFW's scroll wheel callback.
///	Responds to movement of the scroll where.  Allows zooming of the arcball
///	camera.
///
// /////////////////////////////////////////////////////////////////////////////
static void scroll_callback(GLFWwindow* window, double xOffset, double yOffset ) {
    double totChgSq = yOffset;
    arcballCam.cameraAngles.z += totChgSq*0.2f;
    updateCameraDirection();
}

//***********************************************************************************************************************************************************
//
// Setup Functions

// setupGLFW() /////////////////////////////////////////////////////////////////////////////
/// \desc
///		Used to setup everything GLFW related.  This includes the OpenGL context
///	and our window.
/// \return window - the window associated with the new context
// /////////////////////////////////////////////////////////////////////////////
GLFWwindow* setupGLFW() {
    // set what function to use when registering errors
    // this is the ONLY GLFW function that can be called BEFORE GLFW is initialized
    // all other GLFW calls must be performed after GLFW has been initialized
    glfwSetErrorCallback(error_callback);

    // initialize GLFW
    if (!glfwInit()) {
        fprintf( stderr, "[ERROR]: Could not initialize GLFW\n" );
        exit(EXIT_FAILURE);
    } else {
        fprintf( stdout, "[INFO]: GLFW initialized\n" );
    }

    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );						// request forward compatible OpenGL context
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );	        // request OpenGL Core Profile context
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );		                // request OpenGL 4.X context
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );		                // request OpenGL X.1 context
    glfwWindowHint( GLFW_DOUBLEBUFFER, GLU_TRUE );                             // request double buffering

    // create a window for a given size, with a given title
    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Lab10: Geometry Shaders", nullptr, nullptr );
    if( !window ) {						                                        // if the window could not be created, NULL is returned
        fprintf( stderr, "[ERROR]: GLFW Window could not be created\n" );
        glfwTerminate();
        exit( EXIT_FAILURE );
    } else {
        fprintf( stdout, "[INFO]: GLFW Window created\n" );
    }

    glfwMakeContextCurrent(	window );	                                        // make the created window the current window
    glfwSwapInterval( 1 );				                                // update our screen after at least 1 screen refresh

    glfwSetKeyCallback(         window, key_callback		  );            	// set our keyboard callback function
    glfwSetMouseButtonCallback( window, mouse_button_callback );	            // set our mouse button callback function
    glfwSetCursorPosCallback(	window, cursor_callback  	  );	            // set our cursor position callback function
    glfwSetScrollCallback(		window, scroll_callback		  );	            // set our scroll wheel callback function

    return window;										                        // return the window that was created
}

// setupOpenGL() /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Used to setup everything OpenGL related.
///
// /////////////////////////////////////////////////////////////////////////////
void setupOpenGL() {
    glEnable( GL_DEPTH_TEST );					                                // enable depth testing
    glDepthFunc( GL_LESS );							                            // use less than depth test

    glEnable(GL_BLEND);									                        // enable blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	                        // use one minus blending equation

    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );	            // clear the frame buffer to black

    glPointSize( 4.0f );                                                    // make our points bigger (if supported)
}

// setupGLEW() /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Used to initialize GLEW
///
// /////////////////////////////////////////////////////////////////////////////
void setupGLEW() {
    glewExperimental = GL_TRUE;
    GLenum glewResult = glewInit();

    // check for an error
    if( glewResult != GLEW_OK ) {
        fprintf( stderr, "[ERROR]: Error initializing GLEW\n");
        fprintf( stderr, "[ERROR]: %s\n", glewGetErrorString(glewResult) );
        exit(EXIT_FAILURE);
    } else {
        fprintf( stdout, "\n[INFO]: GLEW initialized\n" );
        fprintf( stdout, "[INFO]: Using GLEW %s\n", glewGetString(GLEW_VERSION) );
    }
}

// setupShaders() /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Registers our Shader Programs and query locations
///          of uniform/attribute inputs
///
// /////////////////////////////////////////////////////////////////////////////
void setupShaders() {
    // stuff from lab 8 for skybox and ground
    gouradShaderProgram = new CSCI441::ShaderProgram( "shaders/gouradShader.v.glsl", "shaders/gouradShader.f.glsl" );
    gouradShaderProgramUniforms.mvpMatrix           = gouradShaderProgram->getUniformLocation( "mvpMatrix");
    gouradShaderProgramUniforms.modelMatrix         = gouradShaderProgram->getUniformLocation("modelMatrix");
    gouradShaderProgramUniforms.normalMtx           = gouradShaderProgram->getUniformLocation("normalMtx");
    gouradShaderProgramUniforms.eyePos              = gouradShaderProgram->getUniformLocation("eyePos");
    gouradShaderProgramUniforms.lightPos            = gouradShaderProgram->getUniformLocation("lightPos");
    gouradShaderProgramUniforms.lightDir            = gouradShaderProgram->getUniformLocation("lightDir");
    gouradShaderProgramUniforms.lightCutoff         = gouradShaderProgram->getUniformLocation("lightCutoff");
    gouradShaderProgramUniforms.lightColor          = gouradShaderProgram->getUniformLocation("lightColor");
    gouradShaderProgramUniforms.lightType           = gouradShaderProgram->getUniformLocation("lightType");
    gouradShaderProgramUniforms.materialDiffColor   = gouradShaderProgram->getUniformLocation("materialDiffColor");
    gouradShaderProgramUniforms.materialSpecColor   = gouradShaderProgram->getUniformLocation("materialSpecColor");
    gouradShaderProgramUniforms.materialShininess   = gouradShaderProgram->getUniformLocation("materialShininess");
    gouradShaderProgramUniforms.materialAmbColor    = gouradShaderProgram->getUniformLocation("materialAmbColor");
    gouradShaderProgramAttributes.vPos              = gouradShaderProgram->getAttributeLocation("vPos");
    gouradShaderProgramAttributes.vNormal           = gouradShaderProgram->getAttributeLocation("vNormal");

    texShaderProgram = new CSCI441::ShaderProgram( "shaders/lab06.v.glsl", "shaders/lab06.f.glsl" );
    texShaderProgramUniforms.mvpMatrix   = texShaderProgram->getUniformLocation("mvpMatrix");
    texShaderProgramAttributes.vPos      = texShaderProgram->getAttributeLocation("vPos");
    // TODO #12 lookup the uniform and attribute location
    texShaderProgramUniforms.textureMap  = texShaderProgram->getUniformLocation("textureMap");
    texShaderProgramAttributes.texCoordIn= texShaderProgram->getAttributeLocation("texCoordIn");


    // LOOKHERE #1
    billboardShaderProgram = new CSCI441::ShaderProgram( "shaders/billboardQuadShader.v.glsl",
                                                         "shaders/billboardQuadShader.g.glsl",
                                                         "shaders/billboardQuadShader.f.glsl" );
    billboardShaderProgramUniforms.mvMatrix            = billboardShaderProgram->getUniformLocation( "mvMatrix");
    billboardShaderProgramUniforms.projMatrix          = billboardShaderProgram->getUniformLocation( "projMatrix");
    billboardShaderProgramUniforms.image               = billboardShaderProgram->getUniformLocation( "image");
    billboardShaderProgramAttributes.vPos              = billboardShaderProgram->getAttributeLocation( "vPos");

    billboardShaderProgram->useProgram();
    glUniform1i(billboardShaderProgramUniforms.image, 0);

    fountainShaderUniforms.mvMatrix            = billboardShaderProgram->getUniformLocation( "mvMatrix");
    fountainShaderUniforms.projMatrix          = billboardShaderProgram->getUniformLocation( "projMatrix");
    fountainShaderUniforms.image               = billboardShaderProgram->getUniformLocation( "image");
    fountainShaderAttributes.vPos              = billboardShaderProgram->getAttributeLocation( "vPos");
    fountainShaderAttributes.lifespan          = billboardShaderProgram->getAttributeLocation("lifespan");

    particleSystem.setParticleShaderUandA(*billboardShaderProgram, fountainShaderUniforms, fountainShaderAttributes);

    flatShaderProgram = new CSCI441::ShaderProgram( "shaders/flatShader.v.glsl", "shaders/flatShader.f.glsl" );
    flatShaderProgramUniforms.mvpMatrix             = flatShaderProgram->getUniformLocation("mvpMatrix");
    flatShaderProgramUniforms.color                 = flatShaderProgram->getUniformLocation("color");
    flatShaderProgramAttributes.vPos                = flatShaderProgram->getAttributeLocation("vPos");

    particleSystem.setFlatShaderUandA(*flatShaderProgram,flatShaderProgramUniforms,flatShaderProgramAttributes);

    texShaderProgram->useProgram();                         // set our shader program to be active
    // TODO #13 set the texture map uniform
    glUniform1i(texShaderProgramUniforms.textureMap, 0);

}

// setupBuffers() /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Create our VAOs & VBOs. Send vertex assets to the GPU for future rendering
///
// /////////////////////////////////////////////////////////////////////////////
void setupBuffers() {
    model = new CSCI441::ModelLoader();
    model->loadModelFile( "assets/models/bulb/bulb.obj" );
    // ground
    // ground vbos
    struct Vertex {
        float x, y, z;
        float nx, ny, nz;
        float s, t;
    };

    Vertex platformVertices[4] = {
            { -0.5f, 0.0f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f }, // 0 - BL
            {  0.5f, 0.0f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f }, // 1 - BR
            { -0.5f, 0.0f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f }, // 2 - TL
            {  0.5f, 0.0f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f }  // 3 - TR
    };

    unsigned short platformIndices[4] = { 0, 1, 2, 3 };

    glGenVertexArrays( 1, &platformVAO );
    glBindVertexArray( platformVAO );

    glGenBuffers( 2, platformVBOs );

    glBindBuffer( GL_ARRAY_BUFFER, platformVBOs[0] );
    glBufferData( GL_ARRAY_BUFFER, sizeof( platformVertices ), platformVertices, GL_STATIC_DRAW );

    glEnableVertexAttribArray( gouradShaderProgramAttributes.vPos );
    glVertexAttribPointer( gouradShaderProgramAttributes.vPos, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) 0 );

    glEnableVertexAttribArray( gouradShaderProgramAttributes.vNormal );
    glVertexAttribPointer( gouradShaderProgramAttributes.vNormal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)) );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, platformVBOs[1] );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( platformIndices ), platformIndices, GL_STATIC_DRAW );

    fprintf( stdout, "[INFO]: platform read in with VAO %d\n", platformVAO );


    // skybox
    // skybox vbos
    struct VertexTextured {
        float x, y, z;
        // TODO #14 add texture coordinates to our vertex struct
        float s,t;

    };
    VertexTextured skyBoxFrontVertices[4] = {
            { -1.0f, -1.0f,  1.0f, 0.0f, 0.0f }, // 0 - BL
            {  1.0f, -1.0f,  1.0f, 1.0f, 0.0f }, // 1 - BR
            { -1.0f,  1.0f,  1.0f, 0.0f, 1.0f }, // 2 - TL
            {  1.0f,  1.0f,  1.0f, 1.0f, 1.0f }  // 3 - TR
    };
    VertexTextured skyBoxSideVertices[4] = {
            {  1.0f, -1.0f,  -1.0f, 0.0f, 0.0f }, // 0 - BL
            {  1.0f, -1.0f,  1.0f, 1.0f, 0.0f }, // 1 - BR
            {  1.0f,  1.0f,  -1.0f, 0.0f, 1.0f }, // 2 - TL
            {  1.0f,  1.0f,  1.0f, 1.0f, 1.0f }  // 3 - TR
    };
    VertexTextured skyBoxTopVertices[4] = {
            { -1.0f,  1.0f, -1.0f, 0.0f, 0.0f }, // 0 - BL
            {  1.0f,  1.0f, -1.0f, 1.0f, 0.0f }, // 1 - BR
            { -1.0f,  1.0f,  1.0f, 0.0f, 1.0f }, // 2 - TL
            {  1.0f,  1.0f,  1.0f, 1.0f, 1.0f }  // 3 - TR
    };

    unsigned short skyBoxSidesIndices[4] = { 0, 1, 2, 3 };
    unsigned short skyBoxFrontIndices[4] = { 0, 1, 2, 3 };
    unsigned short skyBoxTopIndices[4] = { 0, 1, 2, 3 };

    glGenVertexArrays( 1, &skyboxFrontVAO );
    glBindVertexArray( skyboxFrontVAO );

    glGenBuffers( 2, skyboxFrontVBOs );

    glBindBuffer( GL_ARRAY_BUFFER, skyboxFrontVBOs[0] );
    glBufferData( GL_ARRAY_BUFFER, sizeof( skyBoxFrontVertices ), skyBoxFrontVertices, GL_STATIC_DRAW );

    glEnableVertexAttribArray( texShaderProgramAttributes.vPos );
    glVertexAttribPointer( texShaderProgramAttributes.vPos, 3, GL_FLOAT, GL_FALSE, sizeof(VertexTextured), (void*) 0 );

    // Repeat TODO #16 to connect this VBO with our shader
    glEnableVertexAttribArray( texShaderProgramAttributes.texCoordIn );
    glVertexAttribPointer( texShaderProgramAttributes.texCoordIn, 2, GL_FLOAT, GL_FALSE, sizeof(VertexTextured), (void*)(sizeof(float) * 3) );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, skyboxFrontVBOs[1] );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( skyBoxFrontIndices ), skyBoxFrontIndices, GL_STATIC_DRAW );

    fprintf( stdout, "[INFO]: quad read in with VAO %d\n\n", skyboxFrontVAO );

    glGenVertexArrays( 1, &skyboxSideVAO );
    glBindVertexArray( skyboxSideVAO );

    glGenBuffers( 2, skyboxSideVBOs );

    glBindBuffer( GL_ARRAY_BUFFER, skyboxSideVBOs[0] );
    glBufferData( GL_ARRAY_BUFFER, sizeof( skyBoxSideVertices ), skyBoxSideVertices, GL_STATIC_DRAW );

    glEnableVertexAttribArray( texShaderProgramAttributes.vPos );
    glVertexAttribPointer( texShaderProgramAttributes.vPos, 3, GL_FLOAT, GL_FALSE, sizeof(VertexTextured), (void*) 0 );

    // Repeat TODO #16 to connect this VBO with our shader
    glEnableVertexAttribArray( texShaderProgramAttributes.texCoordIn );
    glVertexAttribPointer( texShaderProgramAttributes.texCoordIn, 2, GL_FLOAT, GL_FALSE, sizeof(VertexTextured), (void*)(sizeof(float) * 3) );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, skyboxSideVBOs[1] );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( skyBoxSidesIndices ), skyBoxSidesIndices, GL_STATIC_DRAW );

    fprintf( stdout, "[INFO]: quad read in with VAO %d\n\n", skyboxSideVAO );

    glGenVertexArrays( 1, &skyboxTopVAO );
    glBindVertexArray( skyboxTopVAO );

    glGenBuffers( 2, skyboxTopVBOs );

    glBindBuffer( GL_ARRAY_BUFFER, skyboxTopVBOs[0] );
    glBufferData( GL_ARRAY_BUFFER, sizeof( skyBoxTopVertices ), skyBoxTopVertices, GL_STATIC_DRAW );

    glEnableVertexAttribArray( texShaderProgramAttributes.vPos );
    glVertexAttribPointer( texShaderProgramAttributes.vPos, 3, GL_FLOAT, GL_FALSE, sizeof(VertexTextured), (void*) 0 );

    // Repeat TODO #16 to connect this VBO with our shader
    glEnableVertexAttribArray( texShaderProgramAttributes.texCoordIn );
    glVertexAttribPointer( texShaderProgramAttributes.texCoordIn, 2, GL_FLOAT, GL_FALSE, sizeof(VertexTextured), (void*)(sizeof(float) * 3) );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, skyboxTopVBOs[1] );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( skyBoxTopIndices ), skyBoxTopIndices, GL_STATIC_DRAW );

    fprintf( stdout, "[INFO]: quad read in with VAO %d\n\n", skyboxTopVAO );

    particleSystem.initialize(glm::vec3(0,0,0), 1);


}

// setupTextures() /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Register all of our textures with the GPU
///
// /////////////////////////////////////////////////////////////////////////////
void setupTextures() {
    // LOOKHERE #4
    skyboxSidesTextureHandle = CSCI441::TextureUtils::loadAndRegisterTexture("assets/textures/SpaceSkyBox.png");
    skyboxTopTextureHandle = CSCI441::TextureUtils::loadAndRegisterTexture("assets/textures/SpaceSkyBox.png");
    spriteTextureHandle = CSCI441::TextureUtils::loadAndRegisterTexture("assets/textures/snowflake.png");

}

// setupScene() /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Initialize all of our scene information here
///
// /////////////////////////////////////////////////////////////////////////////
void setupScene() {
    // set up mouse info
    leftMouseDown = GL_FALSE;
    controlDown = GL_FALSE;
    mousePosition = glm::vec2( -9999.0f, -9999.0f );
    drawBoundings = false;

    // set up time
    now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    // set up camera info
    arcballCam.cameraAngles   = glm::vec3( 3.52f, 1.9f, 25.0f );
    arcballCam.camDir         = glm::vec3(-1.0f, -1.0f, -1.0f);
    arcballCam.lookAtPoint    = glm::vec3(0.0f, 0.0f, 0.0f);
    arcballCam.upVector       = glm::vec3(    0.0f,  1.0f,  0.0f );

    fountainShaderUniforms.eyePos = arcballCam.eyePos;
    fountainShaderUniforms.lookAtPoint = arcballCam.lookAtPoint;

    updateCameraDirection();

    // set up light info
    glm::vec3 lightColor(1.0f, 1.0f, 0.7f);
    glm::vec3 lightPos(5.0f, 15.0f, 5.0f);
    glm::vec3 lightDir(-1.0f, -3.0f, -1.0f);
    float lightCutoff = glm::cos( glm::radians(7.5f) );
    lightType = 0;
    gouradShaderProgram->useProgram();
    glUniform3fv(gouradShaderProgramUniforms.lightColor, 1, &lightColor[0]);
    glUniform3fv(gouradShaderProgramUniforms.lightPos, 1, &lightPos[0]);
    glUniform3fv(gouradShaderProgramUniforms.lightDir, 1, &lightDir[0]);
    glUniform1f(gouradShaderProgramUniforms.lightCutoff, lightCutoff);
    glUniform1i(gouradShaderProgramUniforms.lightType, lightType);


    // setup snowglobe
    snowglobeAngle = 0.0f;

    //suckable objects:
    myTeapot.color = glm::vec3(.8,0,0);
    myTeapot.spec = glm::vec3(0,.8,0);
    myTeapot.ambient = glm::vec3(0,0,.3);
    myTeapot.transform.position = glm::vec3 (5,5,2);
    myTeapot.velocity = glm::vec3 (-.6,.3,.4);
    myTeapot.transform.rotation = Transform::toQuaternion(0,0,0);
    myTeapot.rotationalVelocity = glm::vec3 (0.1,0,0.05);

    myCube.color = glm::vec3(.8,.3,.4);
    myCube.spec = glm::vec3(.9,.9,.95);
    myCube.ambient = glm::vec3(.7,.7,.7);
    myCube.transform.position = glm::vec3 (0,2,5);
    myCube.velocity = glm::vec3 (.6,-.4,.1);
    myCube.transform.rotation = Transform::toQuaternion(0,0,0);
    myCube.rotationalVelocity = glm::vec3 (-0.1,.02,0);

    myBulb.color = glm::vec3(.8, .4, .0);
    myBulb.spec = glm::vec3(.2, .2, .2);
    myBulb.ambient = glm::vec3(.3, .3, .3);
    myBulb.transform.position = glm::vec3 (1, -4, -3);
    myBulb.velocity = glm::vec3 (-.6, .4, .1);
    myBulb.transform.rotation = Transform::toQuaternion(0, 0, 0);
    myBulb.rotationalVelocity = glm::vec3 (-0.1, .02, 0);
}

// initialize() /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Create our OpenGL context,
///          load all information to the GPU,
///          initialize scene information
/// \return window - the window that was created when the OpenGL context was created
// /////////////////////////////////////////////////////////////////////////////
GLFWwindow* initialize() {
    // GLFW sets up our OpenGL context so must be done first
    GLFWwindow* window = setupGLFW();	                // initialize all of the GLFW specific information related to OpenGL and our window
    setupGLEW();										// initialize all of the GLEW specific information
    setupOpenGL();										// initialize all of the OpenGL specific information

    CSCI441::OpenGLUtils::printOpenGLInfo();            // print our OpenGL information

    setupShaders();                                     // load all of our shader programs onto the GPU and get shader input locations
    setupBuffers();										// load all our VAOs and VBOs onto the GPU
    setupTextures();                                    // load all of our textures onto the GPU
    setupScene();                                       // initialize all of our scene information
    CSCI441::setVertexAttributeLocations( vpos_attrib_location );

    fprintf( stdout, "\n[INFO]: Setup complete\n" );

    return window;
}

//***********************************************************************************************************************************************************
//
// Cleanup Functions

// cleanupShader() /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Delete shaders off of the GPU
///
// /////////////////////////////////////////////////////////////////////////////
void cleanupShaders() {
    fprintf( stdout, "[INFO]: ...deleting shaders.\n" );

    delete gouradShaderProgram;
    delete flatShaderProgram;
    delete texShaderProgram;
    delete billboardShaderProgram;
}

// cleanupBuffers() /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Delete VAOs and VBOs off of the GPU.
///      Remove any buffers from CPU RAM as well.
///
// /////////////////////////////////////////////////////////////////////////////
void cleanupBuffers() {
    fprintf( stdout, "[INFO]: ...deleting IBOs....\n" );

    glDeleteBuffers( NUM_VAOS, ibos );

    fprintf( stdout, "[INFO]: ...deleting VBOs....\n" );

    glDeleteBuffers( NUM_VAOS, vbos );
    CSCI441::deleteObjectVBOs();

    fprintf( stdout, "[INFO]: ...deleting VAOs....\n" );

    glDeleteVertexArrays( NUM_VAOS, vaos );
    CSCI441::deleteObjectVAOs();

    free(spriteLocations);
    free(spriteIndices);
    free(distances);
}

// cleanupTextures() /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Delete textures off of the GPU
///
// /////////////////////////////////////////////////////////////////////////////
void cleanupTextures() {
    fprintf( stdout, "[INFO]: ...deleting textures\n" );

    glDeleteTextures(1, &spriteTextureHandle);
}

void computeAndSendMatrixUniforms(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) {
    glUniformMatrix4fv(gouradShaderProgramUniforms.modelMatrix, 1, GL_FALSE, &modelMtx[0][0]);
    // precompute the Model-View-Projection matrix on the CPU
    glm::mat4 mvpMtx = projMtx * viewMtx * modelMtx;
    // then send it to the shader on the GPU to apply to every vertex
    glUniformMatrix4fv( gouradShaderProgramUniforms.mvpMatrix, 1, GL_FALSE, &mvpMtx[0][0] );

    //  compute and send the normal matrix to the GPU
    glm::mat3 normalMtx	= glm::mat3(glm::transpose(glm::inverse(modelMtx)));
    glUniformMatrix3fv(gouradShaderProgramUniforms.normalMtx,1,GL_FALSE, &normalMtx[0][0]);
}


void SetupSuckable(suckableObject &object, glm::mat4 viewMatrix, glm::mat4 projectionMatrix)  {
    glm::vec3 blackHolePos = glm::vec3 (0,0,0);

    glm::vec3 objectForce = 2.5f * (blackHolePos-object.transform.position);


    //physics:
    object.velocity += (float)(1/(glm::pow(glm::length(objectForce),2))) * (objectForce) ;
    object.transform.position += object.velocity;
    object.transform.setRotation(object.transform.eulerAngles() + object.rotationalVelocity);

    //now set velocity damping:
    //object.velocity *= 0.98f;


    object.transform.updateMatrix();

    glUniform3fv(gouradShaderProgramUniforms.materialAmbColor, 1, &object.ambient[0]);
    glUniform3fv(gouradShaderProgramUniforms.materialDiffColor, 1, &object.color[0]);
    glUniform3fv(gouradShaderProgramUniforms.materialSpecColor, 1, &object.spec[0]);
    glUniform3fv(gouradShaderProgramUniforms.materialShininess, 1, &object.shininess);



    computeAndSendTransformationMatrices(object.transform.getMatrix(), viewMatrix, projectionMatrix,
                                         gouradShaderProgramUniforms.mvpMatrix,
                                         gouradShaderProgramUniforms.modelMatrix,
                                         gouradShaderProgramUniforms.normalMtx);
}

// shutdown() /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Free all memory on the CPU/GPU and close our OpenGL context
///
// /////////////////////////////////////////////////////////////////////////////
void shutdown(GLFWwindow* window) {
    fprintf( stdout, "\n[INFO]: Shutting down.......\n" );
    fprintf( stdout, "[INFO]: ...closing window...\n" );
    glfwDestroyWindow( window );                        // close our window
    cleanupShaders();                                   // delete shaders from GPU
    cleanupBuffers();                                   // delete VAOs/VBOs from GPU
    cleanupTextures();                                  // delete textures from GPU
    particleSystem.cleanup();                           // delete shaders,VAO/VBOs, and textures from particle system
    fprintf( stdout, "[INFO]: ...closing GLFW.....\n" );
    glfwTerminate();						            // shut down GLFW to clean up our context
    fprintf( stdout, "[INFO]: ..shut down complete!\n" );
}

//***********************************************************************************************************************************************************
//
// Rendering / Drawing Functions - this is where the magic happens!

// renderScene() /////////////////////////////////////////////////////////////////////////////
/// \desc
///		This method will contain all of the objects to be drawn.
/// \param viewMatrix - View Matrix for the Camera this scene should be rendered to
/// \param projectionMatrix - Projection Matrix for the Camera this scene should be rendered to
// /////////////////////////////////////////////////////////////////////////////
void renderScene( glm::mat4 viewMatrix, glm::mat4 projectionMatrix ) {
    /// skybox stuff
    texShaderProgram->useProgram();
    glm::mat4 modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(40, 40, 40));

    glm::mat4 mvpMtx = projectionMatrix * viewMatrix * modelMatrix;
    glUniformMatrix4fv(texShaderProgramUniforms.mvpMatrix, 1, GL_FALSE, &mvpMtx[0][0]);

    glBindTexture(GL_TEXTURE_2D, skyboxSidesTextureHandle);

    glBindVertexArray(skyboxFrontVAO);
    glDrawElements( GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, (void*)0 );
    glBindVertexArray(skyboxSideVAO);
    glDrawElements( GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, (void*)0 );

    glBindTexture(GL_TEXTURE_2D, skyboxTopTextureHandle);

    glBindVertexArray(skyboxTopVAO);
    glDrawElements( GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, (void*)0 );

    //back half
    glBindTexture(GL_TEXTURE_2D, skyboxSidesTextureHandle);
    glm::mat4 newModelMatrix = glm::translate(modelMatrix, glm::vec3(0,0,-2));

    mvpMtx = projectionMatrix * viewMatrix * newModelMatrix;
    glUniformMatrix4fv(texShaderProgramUniforms.mvpMatrix, 1, GL_FALSE, &mvpMtx[0][0]);

    glBindVertexArray(skyboxFrontVAO);
    glDrawElements( GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, (void*)0 );

    newModelMatrix = glm::translate(modelMatrix, glm::vec3(-2,0,0));
    mvpMtx = projectionMatrix * viewMatrix * newModelMatrix;
    glUniformMatrix4fv(texShaderProgramUniforms.mvpMatrix, 1, GL_FALSE, &mvpMtx[0][0]);

    glBindVertexArray(skyboxSideVAO);
    glDrawElements( GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, (void*)0 );

    newModelMatrix = glm::translate(modelMatrix, glm::vec3(0,-2,0));
    mvpMtx = projectionMatrix * viewMatrix * newModelMatrix;
    glUniformMatrix4fv(texShaderProgramUniforms.mvpMatrix, 1, GL_FALSE, &mvpMtx[0][0]);
    glBindTexture(GL_TEXTURE_2D, skyboxTopTextureHandle);

    glBindVertexArray(skyboxTopVAO);
    glDrawElements( GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, (void*)0 );



    // ground stuff
    gouradShaderProgram->useProgram();
    // set the eye position - needed for specular reflection
    glUniform3fv(gouradShaderProgramUniforms.eyePos, 1, &(arcballCam.eyePos[0]));

    CSCI441::setVertexAttributeLocations( gouradShaderProgramAttributes.vPos,     // vertex position location
                                          gouradShaderProgramAttributes.vNormal); // vertex normal location

    modelMatrix = glm::mat4(1.0f);

    glm::vec3 groundDiff = glm::vec3(0.07568f, 0.61424f, 0.07568f);
    glm::vec3 groundSpec = glm::vec3(0.633f, 0.727811f, 0.633f);
    glm::vec3 groundAmbi = glm::vec3(0.0215f, 0.1745f, 0.0215f);
    float groundShine = 128.0f * 0.6f;
    glUniform3fv(gouradShaderProgramUniforms.materialAmbColor, 1, &groundAmbi[0]);
    glUniform3fv(gouradShaderProgramUniforms.materialDiffColor, 1, &groundDiff[0]);
    glUniform3fv(gouradShaderProgramUniforms.materialSpecColor, 1, &groundSpec[0]);
    glUniform3fv(gouradShaderProgramUniforms.materialShininess, 1, &groundShine);

    // draw a larger ground plane by translating a single quad across a grid
    /*
    for(int i = -10; i <= 10; i++) {
        for(int j = -10; j <= 10; j++) {
            modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(i, 0, j));
            computeAndSendTransformationMatrices(modelMatrix, viewMatrix, projectionMatrix,
                                                 gouradShaderProgramUniforms.mvpMatrix,
                                                 gouradShaderProgramUniforms.modelMatrix,
                                                 gouradShaderProgramUniforms.normalMtx);
            glBindVertexArray( platformVAO );
            glDrawElements( GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, (void*)0 );
        }
    }
*/

    gouradShaderProgram->useProgram();

    SetupSuckable(myTeapot, viewMatrix, projectionMatrix);
    CSCI441::drawSolidTeapot( 2.0f );
    SetupSuckable(myCube, viewMatrix, projectionMatrix);
    CSCI441::drawSolidCube(1);
    SetupSuckable(myBulb, viewMatrix, projectionMatrix);
    //before we draw bulb, let's set the point light position:
    glUniform3fv(gouradShaderProgramUniforms.lightPos, 1, &myBulb.transform.position[0]);
    //now, let's actually use a different shader for the bulb:
    //flatShaderProgram->useProgram();
    //glUniformMatrix4fv(flatShaderProgramUniforms.mvpMatrix, 1, GLU_FALSE, &myBulb.transform.getMatrix()[0][0]);
    //glUniform3fv(flatShaderProgramUniforms.color, 1, &myBulb.color[0]);
    model->draw( vpos_attrib_location );

    particleSystem.draw(viewMatrix, projectionMatrix);

    if(drawBoundings)
        particleSystem.drawBoundings(viewMatrix,projectionMatrix, modelMatrix);

    // LOOKHERE #3
//    billboardShaderProgram->useProgram();
//    modelMatrix = glm::rotate(glm::mat4(1.0f), snowglobeAngle, CSCI441::Y_AXIS);
//    computeAndSendTransformationMatrices( modelMatrix, viewMatrix, projectionMatrix,
//                                          billboardShaderProgramUniforms.mvMatrix, billboardShaderProgramUniforms.projMatrix);
//    glBindVertexArray( vaos[VAOS.PARTICLE_SYSTEM] );
//    glBindTexture(GL_TEXTURE_2D, spriteTextureHandle);

//    // TODO #1
//    glm::vec3 v = normalize(arcballCam.lookAtPoint - arcballCam.eyePos);    //view vector
//
//    for(int i = 0; i < NUM_SPRITES; i++) {
//        glm::vec3 currentSprite = spriteLocations[spriteIndices[i]];    //sprite position
//        glm::vec4 p = modelMatrix * glm::vec4(currentSprite, 1);    //sprite point
//        glm::vec4 ep = p - glm::vec4(arcballCam.eyePos, 1);         //ep vector
//        distances[i] = glm::dot(glm::vec4(v,0),ep);              //distance vector
//    }

    // TODO #2
    // sort the indices by distance
//    for(int i = 0; i < NUM_SPRITES; i++) {
//        for(int j = 1; j < NUM_SPRITES; j++) {
//            if(distances[j-1] < distances[j]) {
//                float temp = distances[j-1];
//                distances[j-1] = distances[j];
//                distances[j] = temp;
//                temp = spriteIndices[j-1];
//                spriteIndices[j-1] = spriteIndices[j];
//                spriteIndices[j] = temp;
//            }
//        }
//    }

//    for(int i = 1; i < NUM_SPRITES; i++) {
//        if(distances[i-1] < distances[i])
//            printf("uh oh\n");
//    }

    // TODO #3
//    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibos[VAOS.PARTICLE_SYSTEM] );
//    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(GLushort) * NUM_SPRITES, spriteIndices);
//
//    glDrawElements( GL_POINTS, NUM_SPRITES, GL_UNSIGNED_SHORT, (void*)0 );
}

// updateScene() /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Update all of our scene objects - perform animation here
///
// /////////////////////////////////////////////////////////////////////////////



void updateScene() {

    //find time passed since last update
    unsigned long long time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    int timePassed = time - now;
    int timeThroughSecond = time % 1000;
    //if(now/1000 < time/1000)
    //fprintf(stdout, "SECOND PASSED");     //- used for testing math for when to spawn

    now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    particleSystem.update(timePassed, timeThroughSecond, glm::vec3(0,0,0));

    snowglobeAngle += 0.01f;
    if(snowglobeAngle >= 6.28f) {
        snowglobeAngle -= 6.28f;
    }
}

// run() /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Runs our draw loop and renders/updates our scene
/// \param window - window to render the scene to
// /////////////////////////////////////////////////////////////////////////////
void run(GLFWwindow* window) {
    //  This is our draw loop - all rendering is done here.  We use a loop to keep the window open
    //	until the user decides to close the window and quit the program.  Without a loop, the
    //	window will display once and then the program exits.
    while( !glfwWindowShouldClose(window) ) {	        // check if the window was instructed to be closed
        glDrawBuffer( GL_BACK );				        // work with our back frame buffer
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );	// clear the current color contents and depth buffer in the window

        // Get the size of our framebuffer.  Ideally this should be the same dimensions as our window, but
        // when using a Retina display the actual window can be larger than the requested window.  Therefore
        // query what the actual size of the window we are rendering to is.
        GLint framebufferWidth, framebufferHeight;
        glfwGetFramebufferSize( window, &framebufferWidth, &framebufferHeight );

        // update the viewport - tell OpenGL we want to render to the whole window
        glViewport( 0, 0, framebufferWidth, framebufferHeight );

        // set the projection matrix based on the window size
        // use a perspective projection that ranges
        // with a FOV of 45 degrees, for our current aspect ratio, and Z ranges from [0.001, 1000].
        glm::mat4 projectionMatrix = glm::perspective( 45.0f, (GLfloat) WINDOW_WIDTH / (GLfloat) WINDOW_HEIGHT, 0.001f, 100.0f );

        // set up our look at matrix to position our camera
        arcballCam.eyePos = arcballCam.lookAtPoint + arcballCam.camDir * arcballCam.cameraAngles.z;
        glm::mat4 viewMatrix = glm::lookAt( arcballCam.eyePos,
                                            arcballCam.lookAtPoint,
                                            arcballCam.upVector );

        fountainShaderUniforms.eyePos = arcballCam.eyePos;
        fountainShaderUniforms.lookAtPoint = arcballCam.lookAtPoint;
        particleSystem.setCameraVariables(arcballCam.lookAtPoint, arcballCam.eyePos);

        // draw everything to the window
        // pass our view and projection matrices
        renderScene( viewMatrix, projectionMatrix );

        glfwSwapBuffers(window);                        // flush the OpenGL commands and make sure they get rendered!
        glfwPollEvents();				                // check for any events and signal to redraw screen

        updateScene();                                  // update the objects in our scene
    }
}

//**********************************************************************************************************************************************************
//
// Our main function

// main() /////////////////////////////////////////////////////////////////////////////
///
// /////////////////////////////////////////////////////////////////////////////
int main() {
    GLFWwindow *window = initialize();                  // create OpenGL context and setup EVERYTHING for our program
    run(window);                                        // enter our draw loop and run our program
    shutdown(window);                                   // free up all the memory used and close OpenGL context
    return EXIT_SUCCESS;				                // exit our program successfully!
}
