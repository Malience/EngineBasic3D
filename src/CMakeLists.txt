
file(GLOB_RECURSE DEMO_SOURCES *.h *.cpp)

add_executable(EngineBasic3D ${DEMO_SOURCES})

## Internal Dependencies
## External Dependencies
target_link_libraries(EngineBasic3D PRIVATE EDL::graphics_lib)
target_link_libraries(EngineBasic3D PRIVATE EDL::vulkan)
target_link_libraries(EngineBasic3D PRIVATE EDL::debug)
target_link_libraries(EngineBasic3D PRIVATE stb::stb)
target_link_libraries(EngineBasic3D PRIVATE Vulkan::Vulkan)
target_link_libraries(EngineBasic3D PRIVATE shaderc)
target_link_libraries(EngineBasic3D PRIVATE glfw)
target_link_libraries(EngineBasic3D PRIVATE glm::glm)
target_include_directories(EngineBasic3D PRIVATE "../include")