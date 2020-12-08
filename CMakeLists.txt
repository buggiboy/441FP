cmake_minimum_required(VERSION 3.13)
project(lab10)
set(CMAKE_CXX_STANDARD 17)
set(SOURCE_FILES main.cpp ParticleSystem.cpp ParticleSystem.h Particle.cpp Particle.h LightingShaderStructs.h)
add_executable(lab10 ${SOURCE_FILES})

######
# If you are on the Lab Machines, or have installed the OpenGL libraries somewhere
# other than on your path, leave the following two lines uncommented and update
# the path.  The current path is for the lab environments.
#
# If your installations are on your path, then comment these two lines
######

include_directories("Z:/CSCI441/include")
target_link_directories(lab10 PUBLIC "Z:/CSCI441/lib")

# the following line is linking instructions for Windows.  comment if on OS X, otherwise leave uncommented
target_link_libraries(lab10 opengl32 glfw3 glew32.dll gdi32 stbimage)

# the following line is linking instructions for OS X.  uncomment if on OS X, otherwise leave commented
#target_link_libraries(lab10 "-framework OpenGL" glfw3 "-framework Cocoa" "-framework IOKit" "-framework CoreVideo" glew stbimage)