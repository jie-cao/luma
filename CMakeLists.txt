cmake_minimum_required(VERSION 3.25)
project(luma LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LUMA_USE_IMGUI "Build ImGui Creator" ON)

# Fetch Assimp (supports FBX, OBJ, glTF, DAE, and 40+ formats)
include(FetchContent)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)  # Force static library
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)  # Fix for Chinese Windows
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v5.3.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(assimp)

# Fix encoding issues on Chinese Windows
if(MSVC)
    target_compile_options(assimp PRIVATE /wd4819)
endif()

# Fetch stb for image loading (header-only)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(stb)

add_library(luma_core
    engine/foundation/log.cpp
    engine/scene/scene.cpp
    engine/actions/action.cpp
    engine/engine_facade.cpp
    engine/asset/pipeline.cpp
    engine/asset/model_loader.cpp
    engine/renderer/pbr_renderer.cpp
    engine/renderer/rhi/dx12_backend.cpp
    engine/renderer/rhi/metal_backend.cpp
    engine/renderer/rhi/vulkan_backend.cpp
)

target_include_directories(luma_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(luma_core PRIVATE ${assimp_SOURCE_DIR}/include ${assimp_BINARY_DIR}/include ${stb_SOURCE_DIR})
target_link_libraries(luma_core PUBLIC assimp)
if(WIN32)
    target_link_libraries(luma_core PUBLIC d3d12 dxgi d3dcompiler)
endif()

add_executable(luma_demo apps/demo/main.cpp)
target_link_libraries(luma_demo PRIVATE luma_core)

if(WIN32)
    add_executable(luma_dx12_clear apps/dx12_clear/main.cpp)
    target_link_libraries(luma_dx12_clear PRIVATE luma_core d3d12 dxgi)
    target_compile_definitions(luma_dx12_clear PRIVATE UNICODE _UNICODE)
endif()

add_executable(luma_packager tools/packager/main.cpp)
target_link_libraries(luma_packager PRIVATE luma_core)

if(LUMA_USE_IMGUI AND WIN32)
    FetchContent_Declare(
        imgui
        URL https://github.com/ocornut/imgui/archive/refs/tags/v1.89.9.zip
    )
    FetchContent_MakeAvailable(imgui)
    set(IMGUI_SRC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    )
    add_library(imgui STATIC ${IMGUI_SRC})
    target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
    target_link_libraries(imgui PUBLIC d3d12 dxgi)

    add_executable(luma_creator_imgui apps/creator_imgui/main.cpp)
    target_link_libraries(luma_creator_imgui PRIVATE luma_core imgui d3d12 dxgi)
    target_compile_definitions(luma_creator_imgui PRIVATE UNICODE _UNICODE IMGUI_IMPL_WIN32_DISABLE_GAMEPAD)
endif()
