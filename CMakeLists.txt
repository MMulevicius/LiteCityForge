cmake_minimum_required(VERSION 3.16)

# Use the modern MSVC runtime-library setting.
cmake_policy(SET CMP0091 NEW)

project(ProceduralCityGenerator LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Match the existing static MSVC runtime choice.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

option(ENABLE_AVX2 "Build the application with AVX2 instructions" OFF)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(EXT_Depend/glfw-3.3.2)
add_subdirectory(EXT_Depend/glad)
add_subdirectory(EXT_Depend/stb_image)
add_subdirectory(EXT_Depend/stb_truetype)
add_subdirectory(EXT_Depend/glm)
add_subdirectory(EXT_Depend/imgui-docking)
add_subdirectory(EXT_Depend/gl2d)

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

list(APPEND APP_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/EXT_Depend/ImGuiFileDialog-master/ImGuiFileDialog.cpp"
)

add_executable(${PROJECT_NAME} ${APP_SOURCES})

target_compile_definitions(${PROJECT_NAME} PRIVATE
    GLFW_INCLUDE_NONE=1
    PROJECT_ROOT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<BOOL:${MSVC}>:_CRT_SECURE_NO_WARNINGS>
)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/EXT_Depend/ImGuiFileDialog-master"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glm
    glfw
    glad
    stb_image
    stb_truetype
    gl2d
    imgui
)

if(MSVC AND ENABLE_AVX2)
    target_compile_options(${PROJECT_NAME} PRIVATE /arch:AVX2)
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR LANGUAGES CXX)

if(IPO_SUPPORTED)
    set_property(TARGET ${PROJECT_NAME}
        PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
    )
else()
    message(STATUS "IPO/LTO is unavailable: ${IPO_ERROR}")
endif()