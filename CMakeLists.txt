cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(EngineBasic3D C CXX)

find_package(Vulkan REQUIRED FATAL_ERROR)
find_package(glm REQUIRED FATAL_ERROR)
find_package(stb REQUIRED FATAL_ERROR)
find_package(GLFW3 REQUIRED FATAL_ERROR)

## ShaderC
find_path(shaderc_INCLUDE_DIR
    NAMES shaderc/shaderc.h
    HINTS "$ENV{VULKAN_SDK}/include")
find_library(shaderc_LIBRARY
    NAMES shaderc_combined
    HINTS "$ENV{VULKAN_SDK}/lib")

add_library(shaderc STATIC IMPORTED)
set_target_properties(shaderc PROPERTIES IMPORTED_LOCATION_RELEASE "$ENV{VULKAN_SDK}/lib/shaderc_combined.lib")
set_target_properties(shaderc PROPERTIES IMPORTED_LOCATION_DEBUG "$ENV{VULKAN_SDK}/lib/shaderc_combinedd.lib")
set_target_properties(shaderc PROPERTIES IMPORTED_LOCATION_MINSIZEREL "$ENV{VULKAN_SDK}/lib/shaderc_combined.lib")
set_target_properties(shaderc PROPERTIES IMPORTED_LOCATION_RELWITHDEBINFO "$ENV{VULKAN_SDK}/lib/shaderc_combined.lib")
set_target_properties(shaderc PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${shaderc_INCLUDE_DIR}")

add_subdirectory(ext)

if(NOT TARGET EDL::graphics_lib)
	find_package(EDL REQUIRED FATAL_ERROR)
endif()

## Build libraries
add_subdirectory(src)