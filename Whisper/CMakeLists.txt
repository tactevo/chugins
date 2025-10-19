cmake_minimum_required(VERSION 3.24)

# can't do this because whisper's CMake is architecture-dependent
# need to build x86 and arm64 separately
# set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)

project(Whisper)

if(LINUX OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_options(-fPIC)
endif()

add_library(
    ${PROJECT_NAME} SHARED 
    whisper_chugin.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 11
    CXX_EXTENSIONS OFF
    COMPILE_WARNING_AS_ERROR ON
    POSITION_INDEPENDENT_CODE ON
    PREFIX ""
    OUTPUT_NAME ${PROJECT_NAME}
    SUFFIX ".chug"
)

target_include_directories( ${PROJECT_NAME}
    PUBLIC .
    PUBLIC tinycthread/
    PUBLIC whisper_cpp/include
    PUBLIC whisper_cpp/ggml/include
)

add_subdirectory(whisper_cpp)

target_link_libraries( ${PROJECT_NAME} PRIVATE whisper)
