
file(GLOB_RECURSE DEMO_SOURCES *.h *.cpp)

add_executable(EngineBasic3D ${DEMO_SOURCES})

## Internal Dependencies
## External Dependencies
target_link_libraries(EngineBasic3D PRIVATE EXT::jsonh)
target_link_libraries(EngineBasic3D PRIVATE EXT::rapidobj)
target_link_libraries(EngineBasic3D PRIVATE EXT::stb)
target_link_libraries(EngineBasic3D PRIVATE EDL::Graphics)
target_link_libraries(EngineBasic3D PRIVATE EDL::Vulkan)
target_link_libraries(EngineBasic3D PRIVATE EDL::Debug)
target_link_libraries(EngineBasic3D PRIVATE Vulkan::Vulkan)
target_link_libraries(EngineBasic3D PRIVATE shaderc)
target_link_libraries(EngineBasic3D PRIVATE glfw)
target_link_libraries(EngineBasic3D PRIVATE glm::glm)
#target_link_libraries(EngineBasic3D PRIVATE GameNetworkingSockets::GameNetworkingSockets)

if(WIN32)
  target_link_libraries(EngineBasic3D PRIVATE wsock32 ws2_32)
endif()

set_property(TARGET EngineBasic3D PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")