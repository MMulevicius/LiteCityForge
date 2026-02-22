#Version 1.0.0

cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)

option(PRODUCTION_BUILD "Make this a production build" OFF)


set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE) 

if(MSVC) 
add_compile_options(/arch:AVX2) 
endif()

project(LiteCityForge)


set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)


add_subdirectory(EXT_Depend/glfw-3.3.2)			#window oppener
add_subdirectory(EXT_Depend/glad)				#opengl loader
add_subdirectory(EXT_Depend/stb_image)			#loading immaged
add_subdirectory(EXT_Depend/stb_truetype)		#loading ttf files
#add_subdirectory(EXT_Depend/enet-1.3.17)		#networking
#add_subdirectory(EXT_Depend/raudio)			#audio
add_subdirectory(EXT_Depend/glm)				#math
add_subdirectory(EXT_Depend/imgui-docking)		#ui
add_subdirectory(EXT_Depend/gl2d)				#rendering
#add_subdirectory(EXT_Depend/ImGuiFileDialog-master)

file(GLOB_RECURSE MY_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

list(APPEND MY_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/EXT_Depend/ImGuiFileDialog-master/ImGuiFileDialog.cpp"
)


add_executable("${CMAKE_PROJECT_NAME}" ${MY_SOURCES}) 

set_property(TARGET "${CMAKE_PROJECT_NAME}" PROPERTY CXX_STANDARD 17)


target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC GLFW_INCLUDE_NONE=1) 

if(PRODUCTION_BUILD)
	target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC RESOURCES_PATH="./resources/") 

	target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC PRODUCTION_BUILD=1) 

else()
	target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC RESOURCES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/resources/")
	target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC PRODUCTION_BUILD=0) 

endif()

if(MSVC) 

	target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC _CRT_SECURE_NO_WARNINGS)

endif()


target_include_directories("${CMAKE_PROJECT_NAME}" PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    "${CMAKE_CURRENT_SOURCE_DIR}/EXT_Depend/ImGuiFileDialog-master"
)

target_link_libraries("${CMAKE_PROJECT_NAME}" PRIVATE glm glfw 
	glad stb_image stb_truetype gl2d imgui)


