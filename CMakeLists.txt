cmake_minimum_required(VERSION 3.20)
project(MWFree_Cheat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect all .cpp files under src/
file(GLOB_RECURSE SOURCES "src/*.cpp")

# ImGui sources
set(IMGUI_SOURCES
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_win32.cpp
    external/imgui/backends/imgui_impl_dx11.cpp
)

add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${IMGUI_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    src/
    external/imgui/
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d11.lib
    d3dcompiler.lib
    dxgi.lib
    user32.lib
    kernel32.lib
    gdi32.lib
    psapi.lib
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    # UNICODE/WCHAR kapalı — char API kullanıyoruz
)

# Runtime library çakışmasını önle — CMake seviyesinde ayarla
set_property(TARGET ${PROJECT_NAME} PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:/O2>
    $<$<CONFIG:Debug>:/Od /Zi>
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:NDEBUG>
)
