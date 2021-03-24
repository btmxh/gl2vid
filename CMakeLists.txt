cmake_minimum_required(VERSION 3.12)

project(gl2vid C)

add_library(gl2vid ${CMAKE_CURRENT_LIST_DIR}/gl2vid.c)

if(G2V_USE_FFMPEG_ENCODER)
    find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
    find_library(AVCODEC_LIBRARY avcodec)
    find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)
    find_library(AVFORMAT_LIBRARY avformat)
    find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h)
    find_library(AVUTIL_LIBRARY avutil)
    find_path(SWSCALE_INCLUDE_DIR libswscale/swscale.h)
    find_library(SWSCALE_LIBRARY swscale)
    target_include_directories(gl2vid PRIVATE ${AVCODEC_INCLUDE_DIR} ${AVFORMAT_INCLUDE_DIR} ${AVUTIL_INCLUDE_DIR} ${AVDEVICE_INCLUDE_DIR})
    target_link_libraries(gl2vid PRIVATE ${AVCODEC_LIBRARY} ${AVFORMAT_LIBRARY} ${AVUTIL_LIBRARY} ${SWSCALE_LIBRARY})
    list(APPEND gl2vid_DEFINITIONS G2V_USE_FFMPEG_ENCODER)
    message("Using ffmpeg encoder")
endif()

if(G2V_USE_NVIDIA_ENCODER)
    target_include_directories(gl2vid PRIVATE ${NVenc_ROOT}/Interface ${CUDA_INCLUDE_DIRS})
    find_package(CUDA)
    target_link_libraries(gl2vid PRIVATE ${NVenc_ROOT}/Lib/x64/nvencodeapi.lib ${CUDA_CUDA_LIBRARY})
    list(APPEND gl2vid_DEFINITIONS G2V_USE_NVIDIA_ENCODER)
    message("Using NVenc encoder")
endif()

target_compile_definitions(gl2vid PUBLIC ${gl2vid_DEFINITIONS})

if(NOT TARGET glad)
    add_library(glad ${CMAKE_CURRENT_LIST_DIR}/glad/glad.c)
    target_link_libraries(gl2vid PRIVATE glad)
endif()

find_package(glfw3 CONFIG REQUIRED)
target_include_directories(gl2vid PRIVATE ${GLFW_INCLUDE_DIR})
target_link_libraries(gl2vid PRIVATE glfw)

set(gl2vid_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR})
