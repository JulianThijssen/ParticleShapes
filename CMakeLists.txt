# Specifies the CMake version We require
cmake_minimum_required (VERSION 3.8)

# Store the name of our project in a variable called PROJECT
set(PROJECT "ParticleShapes")
# Sets the name of the project for our build
project (${PROJECT} LANGUAGES CXX)

# Set our Includes folder as the place to look for library includes
#include_directories(${CMAKE_SOURCE_DIR}/3rdParty/Includes)
include_directories(${CMAKE_SOURCE_DIR}/ThirdParty/GDT/include)
include_directories(${CMAKE_SOURCE_DIR}/ThirdParty/KHR/include)
include_directories(${CMAKE_SOURCE_DIR}/ThirdParty/FFTW/include)
#include_directories(${CMAKE_SOURCE_DIR}/ThirdParty/AudioFile/include)
include_directories(${CMAKE_SOURCE_DIR}/ThirdParty/SoLoud/include)
include_directories(${CMAKE_SOURCE_DIR}/ThirdParty/Reactive/include)

# Set the source directory as an include folder for relative includes
include_directories(${CMAKE_SOURCE_DIR}/Source)

# Add source subdirectory Which contains the source files
add_subdirectory(Source)

# Specify the name of the project executable and Which sources should be used in the project
add_executable(${PROJECT}
    ${BASE_SOURCES}
)

source_group( Source FILES ${BASE_SOURCES})

# Specify the libraries to use When linking the executable
target_link_libraries(${PROJECT} ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glfw/lib/$<CONFIG>/glfw3dll.lib)
target_link_libraries (${PROJECT} ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/GDT/lib/$<CONFIG>/GDT.lib)
target_link_libraries (${PROJECT} ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/FFTW/lib/libfftw3-3.lib)
target_link_libraries (${PROJECT} ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/SoLoud/lib/$<CONFIG>/soloud_static_x64.lib)
target_link_libraries (${PROJECT} ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/Reactive/lib/$<CONFIG>/Reactive.lib)

# Install binaries
#install(FILES ThirdParty/GLFW/bin/$<CONFIG>/glfw3.dll DESTINATION ${BINARY_DIR}/$<CONFIG>/)
