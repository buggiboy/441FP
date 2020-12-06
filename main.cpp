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

#include <glm/glm.hpp>                  // include GLM libraries
#include <glm/gtc/matrix_transform.hpp> // and matrix functions

#include <cstdio>				        // for printf functionality
#include <cstdlib>				        // for exit functionality

#include <CSCI441/FramebufferUtils.hpp> // assists with FBO error checking
#include <CSCI441/modelLoader.hpp>      // load OBJ files
#include <CSCI441/OpenGLUtils.hpp>      // prints OpenGL information
#include <CSCI441/ShaderProgram.hpp>    // wrapper class for GLSL shader programs
#include <CSCI441/TextureUtils.hpp>     // convenience for loading textures

//***********************************************************************************************************************************************************
//
// Global Parameters

// fix our window to a specific size
const GLint WINDOW_WIDTH = 640, WINDOW_HEIGHT = 640;

// keep track our mouse information
GLboolean controlDown;                  // if the control button was pressed when the mouse was pressed
GLboolean leftMouseDown;                // if the mouse left button is pressed
glm::vec2 mousePosition;                // current mouse position

// keep track of all our camera information
struct CameraParameters {
    glm::vec3 cameraAngles;             // cameraAngles --> x = theta, y = phi, z = radius
    glm::vec3 camDir;                   // direction to the camera
    glm::vec3 eyePos;                   // camera position
    glm::vec3 lookAtPoint;              // location of our object of interest to view
    glm::vec3 upVector;                 // the upVector of our camera
} arcballCam;

// all drawing information
const GLuint NUM_VAOS = 8;
const struct VAO_IDS {
    const GLuint SKYBOX = 0;            // skybox is in order 0-5 (Back, Right, Front, Left, Bottom, Top)
    const GLuint PLATFORM = 6;
    const GLuint TEXTURED_QUAD = 7;
} VAOS;
GLuint vaos[NUM_VAOS];                  // an array of our VAO descriptors
GLuint vbos[NUM_VAOS];                  // an array of our VBO descriptors
GLuint ibos[NUM_VAOS];                  // an array of our IBO descriptors

// skybox information
GLuint skyboxHandles[6];                // all of our skybox texture handles in order 0-5 (Back, Right, Front, Left, Bottom, Top)

// platform information
GLuint platformTextureHandle;           // handle for the platform texture

CSCI441::ModelLoader* townModel = nullptr;  // stores OBJ model

// framebuffer information
GLuint fbo, rbo;                        // handles for the FBO and RBO
const GLint FBO_WIDTH = 1024, FBO_HEIGHT = 1024;  // FBO dimensions
GLuint fboTextureHandle;        // texture handle to render the FBO to

// Texture shader program for skybox and ground
CSCI441::ShaderProgram *textureShaderProgram = nullptr;
struct TextureShaderProgramUniforms {
    GLint mvpMtx;                       // the MVP Matrix to apply
    GLint tex;                          // the texture to apply
} textureShaderProgramUniforms;
struct TextureShaderProgramAttributes {
    GLint vPos;                         // the vertex position
    GLint vTexCoord;                    // the vertex texture coordinate
} textureShaderProgramAttributes;

// Phong shader program for object model
CSCI441::ShaderProgram *modelPhongShaderProgram = nullptr;
struct ModelPhongShaderProgramUniforms {
    GLint modelViewMtx;                 // the ModelView Matrix to apply
    GLint viewMtx;                      // the View Matrix to apply
    GLint mvpMtx;                       // The MVP Matrix to apply
    GLint normalMtx;                    // the Normal Matrix to apply
    GLint materialDiffuse;              // the Material Diffuse property to apply
    GLint materialSpecular;             // the Material Specular property to apply
    GLint materialAmbient;              // the Material Ambient property to apply
    GLint materialShininess;            // the Material Shininess property to apply
    GLint txtr;                         // the texture to apply
} modelPhongShaderProgramUniforms;
struct ModelPhongShaderProgramAttributes {
    GLint vPos;                         // the vertex position
    GLint vNormal;                      // the vertex normal
    GLint vTextureCoord;                // the vertex texture coordinate
} modelPhongShaderProgramAttributes;

// Postprocessing shader program for after effects
CSCI441::ShaderProgram *postprocessingShaderProgram = nullptr;
struct PostprocessingShaderProgramUniforms {
    GLint projectionMtx;                // the Projection Matrix to apply
    GLint fbo;                          // the FBO texture to apply
} postprocessingShaderProgramUniforms;
struct PostprocessingShaderProgramAttributes {
    GLint vPos;                         // the vertex position
    GLint vTextureCoord;                // the vertex texture coordinate
} postprocessingShaderProgramAttributes;

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
    if( arcballCam.cameraAngles.z >= 35.0f ) arcballCam.cameraAngles.z = 35.0f;

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
/// \param MODEL_MATRIX - current Model Matrix
/// \param VIEW_MATRIX - current View Matrix
/// \param PROJECTION_MATRIX - current Projection Matrix
/// \param MODEL_MTX_LOC - location within currently bound shader to send the Model Matrix
/// \param VIEW_MTX_LOC - location within currently bound shader to send the View Matrix
/// \param PROJECTION_MTX_LOC - location within currently bound shader to send the Projection Matrix
/// \param MODELVIEW_MTX_LOC - location within currently bound shader to send the ModelView Matrix
/// \param VIEWPROJECTION_MTX_LOC - location within currently bound shader to send the ViewProjection Matrix
/// \param MVP_MTX_LOC - location within currently bound shader to send the MVP Matrix
/// \param NORMAL_MTX_LOC - location within currently bound shader to send the Normal Matrix
// //////////////////////////////////////////////////////////////////////////////
void computeAndSendTransformationMatrices( const glm::mat4 MODEL_MATRIX, const glm::mat4 VIEW_MATRIX, const glm::mat4 PROJECTION_MATRIX,
                                           const GLint MODEL_MTX_LOC, const GLint VIEW_MTX_LOC, const GLint PROJECTION_MTX_LOC,
                                           const GLint MODELVIEW_MTX_LOC, const GLint VIEWPROJECTION_MTX_LOC,
                                           const GLint MVP_MTX_LOC,
                                           const GLint NORMAL_MTX_LOC) {
    glm::mat4 mvMatrix = VIEW_MATRIX * MODEL_MATRIX;
    glm::mat4 vpMatrix = PROJECTION_MATRIX * VIEW_MATRIX;
    glm::mat4 mvpMatrix = PROJECTION_MATRIX * mvMatrix;
    glm::mat4 normalMatrix = glm::transpose( glm::inverse( mvMatrix ) );

    glUniformMatrix4fv( MODEL_MTX_LOC,          1, GL_FALSE, &MODEL_MATRIX[0][0]      );
    glUniformMatrix4fv( VIEW_MTX_LOC,           1, GL_FALSE, &VIEW_MATRIX[0][0]       );
    glUniformMatrix4fv( PROJECTION_MTX_LOC,     1, GL_FALSE, &PROJECTION_MATRIX[0][0] );
    glUniformMatrix4fv( MODELVIEW_MTX_LOC,      1, GL_FALSE, &mvMatrix[0][0]          );
    glUniformMatrix4fv( VIEWPROJECTION_MTX_LOC, 1, GL_FALSE, &vpMatrix[0][0]          );
    glUniformMatrix4fv( MVP_MTX_LOC,            1, GL_FALSE, &mvpMatrix[0][0]         );
    glUniformMatrix4fv( NORMAL_MTX_LOC,         1, GL_FALSE, &normalMatrix[0][0]      );
}

//***********************************************************************************************************************************************************
//
// Event Callbacks

// /////////////////////////////////////////////////////////////////////////////
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

// /////////////////////////////////////////////////////////////////////////////
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
                glfwSetWindowShouldClose( window, GLFW_TRUE );
                break;

            default: break;
        }
    }
}

// /////////////////////////////////////////////////////////////////////////////
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

// /////////////////////////////////////////////////////////////////////////////
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

// /////////////////////////////////////////////////////////////////////////////
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

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///		Used to setup everything GLFW related.  This includes the OpenGL context
///	and our window.
///
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
    glfwWindowHint( GLFW_DOUBLEBUFFER, GLFW_TRUE );                             // request double buffering
    glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );                               // do not allow the window to be resized

    // create a window for a given size, with a given title
    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Lab12: Framebuffer Objects", nullptr, nullptr );
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

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Used to setup everything OpenGL related.
///
// /////////////////////////////////////////////////////////////////////////////
void setupOpenGL() {
    glEnable( GL_DEPTH_TEST );					                    // enable depth testing
    glDepthFunc( GL_LESS );							                // use less than depth test

    glEnable(GL_BLEND);									            // enable blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	            // use one minus blending equation

    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );	// clear the frame buffer to black
}

// /////////////////////////////////////////////////////////////////////////////
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

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Registers our Shader Programs and query locations
///          of uniform/attribute inputs
///
// /////////////////////////////////////////////////////////////////////////////
void setupShaders() {
    textureShaderProgram = new CSCI441::ShaderProgram( "shaders/textureShader.v.glsl", "shaders/textureShader.f.glsl" );
    textureShaderProgramUniforms.mvpMtx                 = textureShaderProgram->getUniformLocation( "mvpMtx" );
    textureShaderProgramUniforms.tex                    = textureShaderProgram->getUniformLocation( "tex" );
    textureShaderProgramAttributes.vPos			        = textureShaderProgram->getAttributeLocation( "vPos" );
    textureShaderProgramAttributes.vTexCoord            = textureShaderProgram->getAttributeLocation( "vTexCoord" );
    textureShaderProgram->useProgram();
    glUniform1i(textureShaderProgramUniforms.tex, 0);

    modelPhongShaderProgram = new CSCI441::ShaderProgram( "shaders/texturingPhong.v.glsl", "shaders/texturingPhong.f.glsl" );
    modelPhongShaderProgramUniforms.modelViewMtx 	    = modelPhongShaderProgram->getUniformLocation( "modelviewMtx" );
    modelPhongShaderProgramUniforms.viewMtx 		    = modelPhongShaderProgram->getUniformLocation( "viewMtx" );
    modelPhongShaderProgramUniforms.mvpMtx              = modelPhongShaderProgram->getUniformLocation( "mvpMtx" );
    modelPhongShaderProgramUniforms.normalMtx 	        = modelPhongShaderProgram->getUniformLocation( "normalMtx" );
    modelPhongShaderProgramUniforms.materialDiffuse     = modelPhongShaderProgram->getUniformLocation( "materialDiffuse" );
    modelPhongShaderProgramUniforms.materialSpecular    = modelPhongShaderProgram->getUniformLocation( "materialSpecular" );
    modelPhongShaderProgramUniforms.materialAmbient     = modelPhongShaderProgram->getUniformLocation( "materialAmbient" );
    modelPhongShaderProgramUniforms.materialShininess   = modelPhongShaderProgram->getUniformLocation( "materialShininess" );
    modelPhongShaderProgramUniforms.txtr 	            = modelPhongShaderProgram->getUniformLocation( "txtr" );
    modelPhongShaderProgramAttributes.vPos 	            = modelPhongShaderProgram->getAttributeLocation( "vPos" );
    modelPhongShaderProgramAttributes.vNormal 	        = modelPhongShaderProgram->getAttributeLocation( "vNormal" );
    modelPhongShaderProgramAttributes.vTextureCoord     = modelPhongShaderProgram->getAttributeLocation( "vTexCoord" );
    modelPhongShaderProgram->useProgram();
    glUniform1i(modelPhongShaderProgramUniforms.txtr, 0);

    postprocessingShaderProgram = new CSCI441::ShaderProgram( "shaders/grayscale.v.glsl", "shaders/grayscale.f.glsl" );
    postprocessingShaderProgramUniforms.projectionMtx	= postprocessingShaderProgram->getUniformLocation( "projectionMtx" );
    postprocessingShaderProgramUniforms.fbo		        = postprocessingShaderProgram->getUniformLocation( "fbo" );
    postprocessingShaderProgramAttributes.vPos		    = postprocessingShaderProgram->getAttributeLocation( "vPos" );
    postprocessingShaderProgramAttributes.vTextureCoord = postprocessingShaderProgram->getAttributeLocation( "vTexCoord" );
    postprocessingShaderProgram->useProgram();
    glUniform1i(postprocessingShaderProgramUniforms.fbo, 0);
}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Create our VAOs & VBOs. Send vertex assets to the GPU for future rendering
///
// /////////////////////////////////////////////////////////////////////////////
void setupBuffers() {

    // generate ALL VAOs, VBOs, IBOs at once
    glGenVertexArrays( NUM_VAOS, vaos );
    glGenBuffers( NUM_VAOS, vbos );
    glGenBuffers( NUM_VAOS, ibos );

    // ////////////////////////////////////////
    //
    // Model

    townModel = new CSCI441::ModelLoader();
    townModel->loadModelFile( "assets/models/medstreet/medstreet.obj" );

    // ///////////////////////////////////////
    //
    // PLATFORM

    struct VertexTextured {
        glm::vec3 pos;
        glm::vec2 texCoord;
    };

    const GLfloat PLATFORM_SIZE = 20.0f;

    const VertexTextured PLATFORM_VERTICES[4] = {
            {glm::vec3(-PLATFORM_SIZE,  0.0f, -PLATFORM_SIZE),  glm::vec2(0.0f,  0.0f) }, // 0 - BL
            {glm::vec3( PLATFORM_SIZE,  0.0f, -PLATFORM_SIZE),  glm::vec2(1.0f,  0.0f) }, // 1 - BR
            {glm::vec3(-PLATFORM_SIZE,  0.0f,  PLATFORM_SIZE),  glm::vec2(0.0f, -1.0f) }, // 2 - TL
            {glm::vec3( PLATFORM_SIZE,  0.0f,  PLATFORM_SIZE),  glm::vec2(1.0f, -1.0f) }  // 3 - TR
    };

    const unsigned short PLATFORM_INDICES[4] = {0, 1, 2, 3 };

    glBindVertexArray( vaos[VAOS.PLATFORM] );

    glBindBuffer( GL_ARRAY_BUFFER, vbos[VAOS.PLATFORM] );
    glBufferData( GL_ARRAY_BUFFER, sizeof( PLATFORM_VERTICES ), PLATFORM_VERTICES, GL_STATIC_DRAW );

    glEnableVertexAttribArray( textureShaderProgramAttributes.vPos );
    glVertexAttribPointer( textureShaderProgramAttributes.vPos, 3, GL_FLOAT, GL_FALSE, sizeof(VertexTextured), (void*) 0 );

    glEnableVertexAttribArray( textureShaderProgramAttributes.vTexCoord );
    glVertexAttribPointer( textureShaderProgramAttributes.vTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(VertexTextured), (void*) (sizeof(GLfloat) * 3) );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibos[VAOS.PLATFORM] );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( PLATFORM_INDICES ), PLATFORM_INDICES, GL_STATIC_DRAW );

    // ////////////////////////////////////////
    //
    // SKYBOX

    const GLfloat SKYBOX_SIZE = 40.0f;
    const VertexTextured SKYBOX_VERTICES[6][4] = {
            { // back
                    {glm::vec3(-SKYBOX_SIZE, -SKYBOX_SIZE, -SKYBOX_SIZE), glm::vec2( 0.0f, 0.0f) }, // 0 - BL
                    {glm::vec3(-SKYBOX_SIZE, -SKYBOX_SIZE,  SKYBOX_SIZE), glm::vec2(-1.0f, 0.0f) }, // 1 - BR
                    {glm::vec3(-SKYBOX_SIZE,  SKYBOX_SIZE, -SKYBOX_SIZE), glm::vec2( 0.0f, 1.0f) }, // 2 - TL
                    {glm::vec3(-SKYBOX_SIZE,  SKYBOX_SIZE,  SKYBOX_SIZE), glm::vec2(-1.0f, 1.0f) }  // 3 - TR
            },

            { // right
                    {glm::vec3(-SKYBOX_SIZE, -SKYBOX_SIZE,  SKYBOX_SIZE), glm::vec2( 0.0f, 0.0f) }, // 0 - BL
                    {glm::vec3( SKYBOX_SIZE, -SKYBOX_SIZE,  SKYBOX_SIZE), glm::vec2(-1.0f, 0.0f) }, // 1 - BR
                    {glm::vec3(-SKYBOX_SIZE,  SKYBOX_SIZE,  SKYBOX_SIZE), glm::vec2( 0.0f, 1.0f) }, // 2 - TL
                    {glm::vec3( SKYBOX_SIZE,  SKYBOX_SIZE,  SKYBOX_SIZE), glm::vec2(-1.0f, 1.0f) }  // 3 - TR
            },

            { // front
                    { glm::vec3(SKYBOX_SIZE, -SKYBOX_SIZE, -SKYBOX_SIZE), glm::vec2(0.0f,  0.0f) }, // 0 - BL
                    { glm::vec3(SKYBOX_SIZE, -SKYBOX_SIZE,  SKYBOX_SIZE), glm::vec2(1.0f,  0.0f) }, // 1 - BR
                    { glm::vec3(SKYBOX_SIZE,  SKYBOX_SIZE, -SKYBOX_SIZE), glm::vec2(0.0f,  1.0f) }, // 2 - TL
                    { glm::vec3(SKYBOX_SIZE,  SKYBOX_SIZE,  SKYBOX_SIZE), glm::vec2(1.0f,  1.0f) }  // 3 - TR
            },

            { // left
                    {glm::vec3(-SKYBOX_SIZE, -SKYBOX_SIZE, -SKYBOX_SIZE), glm::vec2(0.0f,  0.0f) }, // 0 - BL
                    {glm::vec3( SKYBOX_SIZE, -SKYBOX_SIZE, -SKYBOX_SIZE), glm::vec2(1.0f,  0.0f) }, // 1 - BR
                    {glm::vec3(-SKYBOX_SIZE,  SKYBOX_SIZE, -SKYBOX_SIZE), glm::vec2(0.0f,  1.0f) }, // 2 - TL
                    {glm::vec3( SKYBOX_SIZE,  SKYBOX_SIZE, -SKYBOX_SIZE), glm::vec2(1.0f,  1.0f) }  // 3 - TR
            },

            { // bottom
                    {glm::vec3(-SKYBOX_SIZE, -SKYBOX_SIZE, -SKYBOX_SIZE), glm::vec2(0.0f,  0.0f) }, // 0 - BL
                    {glm::vec3( SKYBOX_SIZE, -SKYBOX_SIZE, -SKYBOX_SIZE), glm::vec2(0.0f,  1.0f) }, // 1 - BR
                    {glm::vec3(-SKYBOX_SIZE, -SKYBOX_SIZE,  SKYBOX_SIZE), glm::vec2(1.0f,  0.0f) }, // 2 - TL
                    {glm::vec3( SKYBOX_SIZE, -SKYBOX_SIZE,  SKYBOX_SIZE), glm::vec2(1.0f,  1.0f) }  // 3 - TR
            },

            { // top
                    {glm::vec3(-SKYBOX_SIZE,  SKYBOX_SIZE, -SKYBOX_SIZE), glm::vec2(-1.0f, 1.0f) }, // 0 - BL
                    {glm::vec3( SKYBOX_SIZE,  SKYBOX_SIZE, -SKYBOX_SIZE), glm::vec2(-1.0f, 0.0f) }, // 1 - BR
                    {glm::vec3(-SKYBOX_SIZE,  SKYBOX_SIZE,  SKYBOX_SIZE), glm::vec2( 0.0f, 1.0f) }, // 2 - TL
                    {glm::vec3( SKYBOX_SIZE,  SKYBOX_SIZE,  SKYBOX_SIZE), glm::vec2( 0.0f, 0.0f) }  // 3 - TR
            }
    };

    const unsigned short SKYBOX_INDICES[4] = {0, 1, 2, 3 };

    for( int i = 0; i < 6; i++ ) {
        glBindVertexArray( vaos[VAOS.SKYBOX + i] );

        glBindBuffer( GL_ARRAY_BUFFER, vbos[VAOS.SKYBOX + i] );
        glBufferData( GL_ARRAY_BUFFER, sizeof(SKYBOX_VERTICES[i]), SKYBOX_VERTICES[i], GL_STATIC_DRAW );

        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibos[VAOS.SKYBOX + i] );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(SKYBOX_INDICES), SKYBOX_INDICES, GL_STATIC_DRAW );

        glEnableVertexAttribArray( textureShaderProgramAttributes.vPos );
        glVertexAttribPointer( textureShaderProgramAttributes.vPos, 3, GL_FLOAT, GL_FALSE, sizeof(VertexTextured), (void*) 0 );

        glEnableVertexAttribArray( textureShaderProgramAttributes.vTexCoord );
        glVertexAttribPointer( textureShaderProgramAttributes.vTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(VertexTextured), (void*) (sizeof(GLfloat) * 3) );
    }

    // ////////////////////////////////////////
    //
    // TEXTURED QUAD - LOOKHERE #1

    const VertexTextured TEXTURED_QUAD_VERTICES[4] = {
            { glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec2(0.0f,  0.0f) }, // 0 - BL
            { glm::vec3( 1.0f, -1.0f, 0.0f), glm::vec2(1.0f,  0.0f) }, // 1 - BR
            { glm::vec3(-1.0f,  1.0f, 0.0f), glm::vec2(0.0f,  1.0f) }, // 2 - TL
            { glm::vec3( 1.0f,  1.0f, 0.0f), glm::vec2(1.0f,  1.0f) }  // 3 - TR
    };

    const unsigned short TEXTURED_QUAD_INDICES[4] = {0, 1, 2, 3 };

    glBindVertexArray( vaos[VAOS.TEXTURED_QUAD] );

    glBindBuffer( GL_ARRAY_BUFFER, vbos[VAOS.TEXTURED_QUAD] );
    glBufferData( GL_ARRAY_BUFFER, sizeof(TEXTURED_QUAD_VERTICES), TEXTURED_QUAD_VERTICES, GL_STATIC_DRAW );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibos[VAOS.TEXTURED_QUAD] );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(TEXTURED_QUAD_INDICES), TEXTURED_QUAD_INDICES, GL_STATIC_DRAW );

    glEnableVertexAttribArray( postprocessingShaderProgramAttributes.vPos );
    glVertexAttribPointer(postprocessingShaderProgramAttributes.vPos, 3, GL_FLOAT, GL_FALSE, sizeof(VertexTextured), (void*) 0);

    glEnableVertexAttribArray(postprocessingShaderProgramAttributes.vTextureCoord);
    glVertexAttribPointer(postprocessingShaderProgramAttributes.vTextureCoord, 2, GL_FLOAT, GL_FALSE, sizeof(VertexTextured), (void*) (sizeof(GLfloat) * 3));
}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Register all of our textures with the GPU
///
// /////////////////////////////////////////////////////////////////////////////
void setupTextures() {
    platformTextureHandle = CSCI441::TextureUtils::loadAndRegisterTexture( "assets/textures/ground.png" );

    // get handles for our full skybox
    printf( "[INFO]: registering skybox...\n" );
    fflush( stdout );
    skyboxHandles[0] = CSCI441::TextureUtils::loadAndRegisterTexture( "assets/textures/skybox/DOOM16BK.png"   );
    skyboxHandles[1] = CSCI441::TextureUtils::loadAndRegisterTexture( "assets/textures/skybox/DOOM16RT.png"   );
    skyboxHandles[2] = CSCI441::TextureUtils::loadAndRegisterTexture( "assets/textures/skybox/DOOM16FT.png"  );
    skyboxHandles[3] = CSCI441::TextureUtils::loadAndRegisterTexture( "assets/textures/skybox/DOOM16LF.png"  );
    skyboxHandles[4] = CSCI441::TextureUtils::loadAndRegisterTexture( "assets/textures/skybox/DOOM16DN.png" );
    skyboxHandles[5] = CSCI441::TextureUtils::loadAndRegisterTexture( "assets/textures/skybox/DOOM16UP.png"    );
    printf( "[INFO]: skybox textures read in and registered!\n\n" );
}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Initialize our Framebuffer here
///
// /////////////////////////////////////////////////////////////////////////////
void setupFramebuffers() {
    // TODO #1 set up the framebuffer object!
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, FBO_WIDTH, FBO_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
    glGenTextures(1, &fboTextureHandle);
    glBindTexture(GL_TEXTURE_2D, fboTextureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, FBO_WIDTH, FBO_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTextureHandle, 0);
    CSCI441::FramebufferUtils::printFramebufferStatusMessage(GL_FRAMEBUFFER);
    CSCI441::FramebufferUtils::printFramebufferInfo(GL_FRAMEBUFFER, fbo);
}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Initialize all of our scene information here
///
// /////////////////////////////////////////////////////////////////////////////
void setupScene() {
    // set up mouse info
    leftMouseDown = GL_FALSE;
    controlDown = GL_FALSE;
    mousePosition = glm::vec2( -9999.0f, -9999.0f );

    // set up camera info
    arcballCam.cameraAngles   = glm::vec3( 3.52f, 1.9f, 25.0f );
    arcballCam.camDir         = glm::vec3(-1.0f, -1.0f, -1.0f);
    arcballCam.lookAtPoint    = glm::vec3(0.0f, 0.0f, 0.0f);
    arcballCam.upVector       = glm::vec3(    0.0f,  1.0f,  0.0f );
    updateCameraDirection();
}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Create our OpenGL context,
///          load all information to the GPU,
///          initialize scene information
///
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
    setupFramebuffers();                                // initialize our FBOs on the GPU
    setupScene();                                       // initialize all of our scene information

    fprintf( stdout, "\n[INFO]: Setup complete\n" );

    return window;
}

//***********************************************************************************************************************************************************
//
// Cleanup Functions

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Delete shaders off of the GPU
///
// /////////////////////////////////////////////////////////////////////////////
void cleanupShaders() {
    fprintf( stdout, "[INFO]: ...deleting shaders.\n" );

    delete textureShaderProgram;
    delete modelPhongShaderProgram;
    delete postprocessingShaderProgram;
}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Delete VAOs and VBOs off of the GPU
///
// /////////////////////////////////////////////////////////////////////////////
void cleanupBuffers() {
    fprintf( stdout, "[INFO]: ...deleting IBOs....\n" );

    glDeleteBuffers( NUM_VAOS, ibos );

    fprintf( stdout, "[INFO]: ...deleting VBOs....\n" );

    glDeleteBuffers( NUM_VAOS, vbos );

    fprintf( stdout, "[INFO]: ...deleting VAOs....\n" );

    glDeleteVertexArrays( NUM_VAOS, vaos );
}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Delete textures off of the GPU
///
// /////////////////////////////////////////////////////////////////////////////
void cleanupTextures() {
    fprintf( stdout, "[INFO]: ...deleting textures\n" );

    glDeleteTextures(1, &platformTextureHandle);
    glDeleteTextures(6, skyboxHandles);
}

void cleanupFramebuffers() {
    fprintf( stdout, "[INFO]: ...deleting FBOs....\n");
    glDeleteFramebuffers(1, &fbo);                          // delete the FBO
    glDeleteTextures(1, &fboTextureHandle);     // and the associated texture for it

    fprintf( stdout, "[INFO]: ...deleting RBOs....\n");
    glDeleteRenderbuffers(1, &rbo);                         // plus the RBO
}

// /////////////////////////////////////////////////////////////////////////////
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
    cleanupFramebuffers();                              // delete FBOs from GPU
    fprintf( stdout, "[INFO]: ...closing GLFW.....\n" );
    glfwTerminate();						            // shut down GLFW to clean up our context
    fprintf( stdout, "[INFO]: ..shut down complete!\n" );
}

//***********************************************************************************************************************************************************
//
// Rendering / Drawing Functions - this is where the magic happens!

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///		This method will contain all of the objects to be drawn.
/// \param viewMatrix - View Matrix for the Camera this scene should be rendered to
/// \param projectionMatrix - Projection Matrix for the Camera this scene should be rendered to
// /////////////////////////////////////////////////////////////////////////////
void renderScene( glm::mat4 viewMatrix, glm::mat4 projectionMatrix ) {
    glm::mat4 modelMatrix = glm::mat4( 1.0f );

    // ///////////////////////
    //
    // Draw Textured Skybox

    textureShaderProgram->useProgram();
    computeAndSendTransformationMatrices(modelMatrix, viewMatrix, projectionMatrix,
                                         -1, -1, -1,
                                         -1, -1,
                                         textureShaderProgramUniforms.mvpMtx,
                                         -1);

    for( unsigned int i = 0; i < 6; i++ ) {
        glBindVertexArray( vaos[VAOS.SKYBOX + i] );
        glBindTexture( GL_TEXTURE_2D, skyboxHandles[i] );
        glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, (void*)0);
    }

    // ///////////////////////
    //
    // Draw Textured Platform

    glBindVertexArray( vaos[VAOS.PLATFORM] );
    glBindTexture( GL_TEXTURE_2D, platformTextureHandle );
    glDrawElements( GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, (void*)0 );

    // ///////////////////////
    //
    // Draw Object Model with Phong Shading using Blinn-Phong Reflectance & Texturing

    modelPhongShaderProgram->useProgram();
    modelMatrix = glm::translate( glm::mat4(1.0f), glm::vec3(4, 0.1, 0) );

    computeAndSendTransformationMatrices(modelMatrix, viewMatrix, projectionMatrix,
                                         -1, modelPhongShaderProgramUniforms.viewMtx, -1,
                                         modelPhongShaderProgramUniforms.modelViewMtx, -1,
                                         modelPhongShaderProgramUniforms.mvpMtx,
                                         modelPhongShaderProgramUniforms.normalMtx);

    townModel->draw( modelPhongShaderProgramAttributes.vPos, modelPhongShaderProgramAttributes.vNormal, modelPhongShaderProgramAttributes.vTextureCoord,
                 modelPhongShaderProgramUniforms.materialDiffuse, modelPhongShaderProgramUniforms.materialSpecular, modelPhongShaderProgramUniforms.materialShininess, modelPhongShaderProgramUniforms.materialAmbient,
                 GL_TEXTURE0 );
}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Update all of our scene objects - perform animation here
///
// /////////////////////////////////////////////////////////////////////////////
void updateScene() {

}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///     Renders our scene as normal with all objects displayed
// /////////////////////////////////////////////////////////////////////////////
void firstPass(GLFWwindow* window) {
    glDrawBuffer( GL_BACK );				        // work with our back frame buffer
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );	// clear the current color contents and depth buffer in the window

    // Get the size of our framebuffer.  Ideally this should be the same dimensions as our window, but
    // when using a Retina display the actual window can be larger than the requested window.  Therefore
    // query what the actual size of the window we are rendering to is.
    GLint windowWidth, windowHeight;
    glfwGetFramebufferSize( window, &windowWidth, &windowHeight );

    // TODO #2B
    glViewport( 0, 0, FBO_WIDTH, FBO_HEIGHT );

    // set the projection matrix based on the window size
    // use a perspective projection that ranges
    // with a FOV of 45 degrees, for our current aspect ratio, and Z ranges from [0.001, 1000].
    glm::mat4 projectionMatrix = glm::perspective( 45.0f, (GLfloat) FBO_WIDTH / (GLfloat) FBO_HEIGHT, 0.001f, 100.0f );

    // set up our look at matrix to position our camera
    arcballCam.eyePos = arcballCam.lookAtPoint + arcballCam.camDir * arcballCam.cameraAngles.z;
    glm::mat4 viewMatrix = glm::lookAt( arcballCam.eyePos,
                                        arcballCam.lookAtPoint,
                                        arcballCam.upVector );

    // draw everything to the window
    // pass our view and projection matrices
    renderScene( viewMatrix, projectionMatrix );
}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///     Renders a full screen textured quad with the FBO overlaid
/// \param window The window to render to
// /////////////////////////////////////////////////////////////////////////////
void secondPass(GLFWwindow* window) {
    glDrawBuffer( GL_BACK );				                     // work with our back frame buffer
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );	 // clear the current color contents and depth buffer in the window

    // Get the size of our framebuffer.  Ideally this should be the same dimensions as our window, but
    // when using a Retina display the actual window can be larger than the requested window.  Therefore
    // query what the actual size of the window we are rendering to is.
    GLint windowWidth, windowHeight;
    glfwGetFramebufferSize( window, &windowWidth, &windowHeight );

    // update the viewport - tell OpenGL we want to render to the whole window
    glViewport( 0, 0, windowWidth, windowHeight );

    // TODO #3B
    glm::mat4 projMtx = glm::ortho(-1, 1, -1, 1, -1, 1);
    postprocessingShaderProgram->useProgram();
    glUniformMatrix4fv(postprocessingShaderProgramUniforms.projectionMtx, 1, GL_FALSE, &projMtx[0][0]);
    glBindTexture(GL_TEXTURE_2D, fboTextureHandle);
    glBindVertexArray(vaos[7]);
    glDrawElements(GL_TRIANGLE_STRIP, 4,  GL_UNSIGNED_SHORT, (void*)0);

}

// /////////////////////////////////////////////////////////////////////////////
/// \desc
///      Runs our draw loop and renders/updates our scene
/// \param window - window to render the scene to
// /////////////////////////////////////////////////////////////////////////////
void run(GLFWwindow* window) {
    //  This is our draw loop - all rendering is done here.  We use a loop to keep the window open
    //	until the user decides to close the window and quit the program.  Without a loop, the
    //	window will display once and then the program exits.
    while( !glfwWindowShouldClose(window) ) {	        // check if the window was instructed to be closed

        // /////////////////
        //
        // FIRST PASS
        // TODO #2A
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        firstPass(window);                              // render our scene as normal with all our objects
        // TODO #2C
        glFlush();

        // /////////////////
        //
        // SECOND PASS
        // TODO #3A
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        secondPass(window);

        glfwSwapBuffers(window);                        // flush the OpenGL commands and make sure they get rendered!
        glfwPollEvents();				                // check for any events and signal to redraw screen

        updateScene();                                  // update the objects in our scene
    }
}

//**********************************************************************************************************************************************************
//
// Our main function

// /////////////////////////////////////////////////////////////////////////////
///
// /////////////////////////////////////////////////////////////////////////////
int main() {
    GLFWwindow *window = initialize();                  // create OpenGL context and setup EVERYTHING for our program
    run(window);                                        // enter our draw loop and run our program
    shutdown(window);                                   // free up all the memory used and close OpenGL context
    return EXIT_SUCCESS;				                // exit our program successfully!
}
