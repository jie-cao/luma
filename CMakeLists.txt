cmake_minimum_required(VERSION 3.25)

# Enable Objective-C++ for macOS/iOS Metal support
if(APPLE)
    project(luma LANGUAGES CXX OBJCXX)
else()
    project(luma LANGUAGES CXX)
endif()

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
set(ASSIMP_BUILD_ZLIB OFF CACHE BOOL "" FORCE)  # Use system zlib on macOS
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v5.4.3
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

# ===== Core Engine Library =====
set(LUMA_CORE_SOURCES
    engine/foundation/log.cpp
    engine/scene/scene.cpp
    engine/actions/action.cpp
    engine/engine_facade.cpp
    engine/asset/pipeline.cpp
    engine/asset/model_loader.cpp
    engine/asset/async_texture_loader.cpp
    engine/asset/hdr_loader.cpp
    engine/renderer/ibl_generator.cpp
    engine/util/file_watcher.cpp
)

# Platform-specific renderer sources (UnifiedRenderer)
if(WIN32)
    list(APPEND LUMA_CORE_SOURCES
        engine/renderer/unified_renderer_dx12.cpp
    )
elseif(APPLE)
    list(APPEND LUMA_CORE_SOURCES
        engine/renderer/unified_renderer_metal.mm
    )
endif()

add_library(luma_core ${LUMA_CORE_SOURCES})

target_include_directories(luma_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(luma_core PRIVATE ${assimp_SOURCE_DIR}/include ${assimp_BINARY_DIR}/include ${stb_SOURCE_DIR})
target_link_libraries(luma_core PUBLIC assimp)

# Platform-specific linking
if(WIN32)
    target_link_libraries(luma_core PUBLIC d3d12 dxgi d3dcompiler)
elseif(APPLE)
    find_library(METAL_FRAMEWORK Metal REQUIRED)
    find_library(METALKIT_FRAMEWORK MetalKit REQUIRED)
    find_library(QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)
    find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
    find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
    target_link_libraries(luma_core PUBLIC 
        ${METAL_FRAMEWORK}
        ${METALKIT_FRAMEWORK}
        ${QUARTZCORE_FRAMEWORK}
        ${COCOA_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
    )
    # Enable ARC for Objective-C++ files
    set_source_files_properties(
        engine/renderer/unified_renderer_metal.mm
        PROPERTIES COMPILE_FLAGS "-fobjc-arc"
    )
endif()

# ===== Animation Test =====
add_executable(luma_anim_test main.cpp)
target_link_libraries(luma_anim_test PRIVATE luma_core)

# ===== Integration Test =====
add_executable(luma_integration_test tests/run_tests.cpp)
target_include_directories(luma_integration_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(luma_integration_test PRIVATE luma_core)

# ===== Demo App =====
add_executable(luma_demo apps/demo/main.cpp)
target_link_libraries(luma_demo PRIVATE luma_core)

# ===== Packager Tool =====
add_executable(luma_packager tools/packager/main.cpp)
target_link_libraries(luma_packager PRIVATE luma_core)

# ===== Windows DX12 Clear Example =====
if(WIN32)
    add_executable(luma_dx12_clear apps/dx12_clear/main.cpp)
    target_link_libraries(luma_dx12_clear PRIVATE luma_core d3d12 dxgi)
    target_compile_definitions(luma_dx12_clear PRIVATE UNICODE _UNICODE)
endif()

# ===== Windows LUMA Studio =====
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

    add_executable(luma_studio apps/luma_studio/main.cpp)
    target_link_libraries(luma_studio PRIVATE luma_core imgui d3d12 dxgi)
    target_compile_definitions(luma_studio PRIVATE UNICODE _UNICODE IMGUI_IMPL_WIN32_DISABLE_GAMEPAD)
endif()

# ===== macOS LUMA Studio =====
if(APPLE)
    # Fetch ImGui for macOS Metal
    FetchContent_Declare(
        imgui
        URL https://github.com/ocornut/imgui/archive/refs/tags/v1.89.9.zip
    )
    FetchContent_MakeAvailable(imgui)
    
    # ImGui Metal library
    set(IMGUI_METAL_SRC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_metal.mm
        ${imgui_SOURCE_DIR}/backends/imgui_impl_osx.mm
    )
    add_library(imgui_metal STATIC ${IMGUI_METAL_SRC})
    target_include_directories(imgui_metal PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
    find_library(GAMECONTROLLER_FRAMEWORK GameController REQUIRED)
    target_link_libraries(imgui_metal PUBLIC 
        ${METAL_FRAMEWORK}
        ${COCOA_FRAMEWORK}
        ${GAMECONTROLLER_FRAMEWORK}
    )
    set_source_files_properties(
        ${imgui_SOURCE_DIR}/backends/imgui_impl_metal.mm
        ${imgui_SOURCE_DIR}/backends/imgui_impl_osx.mm
        PROPERTIES COMPILE_FLAGS "-fobjc-arc"
    )

    # macOS Application Bundle
    add_executable(luma_studio_macos MACOSX_BUNDLE
        apps/luma_studio_macos/main.mm
        apps/luma_studio_macos/AppDelegate.mm
        apps/luma_studio_macos/LumaView.mm
    )
    
    set_target_properties(luma_studio_macos PROPERTIES
        BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.luma.studio"
        MACOSX_BUNDLE_BUNDLE_NAME "LUMA Studio"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/apps/luma_studio_macos/Info.plist"
    )
    
    target_include_directories(luma_studio_macos PRIVATE ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
    
    target_link_libraries(luma_studio_macos PRIVATE 
        luma_core 
        imgui_metal
        ${METAL_FRAMEWORK}
        ${METALKIT_FRAMEWORK}
        ${QUARTZCORE_FRAMEWORK}
        ${COCOA_FRAMEWORK}
    )
    
    # Enable ARC for Objective-C++ files
    set_source_files_properties(
        apps/luma_studio_macos/main.mm
        apps/luma_studio_macos/AppDelegate.mm
        apps/luma_studio_macos/LumaView.mm
        PROPERTIES COMPILE_FLAGS "-fobjc-arc"
    )
    
    # Copy Metal shader to app bundle
    set(SHADER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/engine/renderer/shaders/pbr.metal")
    add_custom_command(TARGET luma_studio_macos POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SHADER_SOURCE}"
            "$<TARGET_BUNDLE_CONTENT_DIR:luma_studio_macos>/Resources/pbr.metal"
        COMMENT "Copying Metal shader to bundle"
    )
endif()
