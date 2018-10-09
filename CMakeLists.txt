cmake_minimum_required(VERSION 3.10)
project(GPGPU)

set(CMAKE_CXX_STANDARD 11)

add_executable(ray_trace
        src/main.cpp
        src/OpenCL.h src/Log.cpp src/Log.h src/OpenCL.cpp src/lodepng/lodepng.h src/lodepng/lodepng.cpp
        src/Octree.cpp src/Octree.h)

target_link_libraries(ray_trace glfw GLEW GL OpenCL)
add_subdirectory(src/glm)
