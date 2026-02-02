# LUMA Viewer Android - Native CMake
cmake_minimum_required(VERSION 3.22.1)

project("luma_viewer" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# LUMA Engine root directory
set(LUMA_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../..")

# Source files
add_library(${CMAKE_PROJECT_NAME} SHARED
    vulkan_renderer.cpp
    jni_bridge.cpp
)

# Include directories
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${LUMA_ROOT}
    ${ANDROID_NDK}/sources/third_party/vulkan/src/include
)

# Link libraries
find_library(log-lib log)
find_library(android-lib android)

target_link_libraries(${CMAKE_PROJECT_NAME}
    ${log-lib}
    ${android-lib}
    vulkan
)
