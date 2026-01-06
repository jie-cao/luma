cmake_minimum_required(VERSION 3.25)
project(luma LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LUMA_WITH_TINYGLTF "Enable tinygltf parsing" OFF)
option(LUMA_USE_IMGUI "Build ImGui Creator" ON)

add_library(luma_core
    engine/foundation/log.cpp
    engine/scene/scene.cpp
    engine/actions/action.cpp
    engine/engine_facade.cpp
    engine/asset/pipeline.cpp
    engine/renderer/rhi/dx12_backend.cpp
    engine/renderer/rhi/metal_backend.cpp
    engine/renderer/rhi/vulkan_backend.cpp
)

target_include_directories(luma_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(LUMA_WITH_TINYGLTF)
    target_compile_definitions(luma_core PUBLIC LUMA_WITH_TINYGLTF)
    target_include_directories(luma_core PRIVATE third_party/tinygltf)
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
    include(FetchContent)
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
