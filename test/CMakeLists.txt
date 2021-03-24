cmake_minimum_required(VERSION 3.12)

project(g2v_test C)

option(G2V_USE_FFMPEG_ENCODER "" ON)
include(${CMAKE_CURRENT_SOURCE_DIR}/../CMakeLists.txt)

add_executable(test test.c)

# TODO: refactor this crap, since literally no libraries do this
target_compile_definitions(test PUBLIC ${gl2vid_DEFINITIONS})

target_include_directories(test PUBLIC ${gl2vid_INCLUDE_DIR})

# glad embedded with gl2vid support only OpenGL 3.0, to use other features please define your own glad library
# Note: If you uses glew, gl3w, etc. may cause link errors
target_link_libraries(test PUBLIC glad)
target_link_libraries(test PUBLIC gl2vid)
