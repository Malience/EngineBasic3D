# EngineBasic3D
A simple implementation of the <a href="https://github.com/Malience/EngineDevelopmentLibrary" target="_blank">`Engine Development Library`</a>.

Includes examples of modern rendering techniques such as:
* Data-Oriented Design
* Modern GLSL
* Fully Bindless Rendering
* Buffer References
* Mesh Shading
* Vulkan 1.3
* Nearless Depth Buffer (Depth values stored as distance instead)
* Per-Triangle Materials

Contains asset loading methods for:
* OBJ Wavefront files
* JSON files
** Scene Files
** Custom JSON Files
* Image Files
* GLSL Mesh, Vertex, and Fragment Shaders

Future Plans:
* Clustered Forward Rendering
* Ray-Traced Hard Shadows
* Subsurface Scattering
* More Advanced Distance Buffer (including a proper distance algorithm and possibly integer casting)

Depth Buffer:
In this implementation the depth buffer has been replaced with a distance buffer. Rather than storing some depth value it stores the distance from the camera. This allows the depth buffer to retain more accuracy when converting from depth to position. Necessary for Ray-Tracing with Forward rendering.

## Build
1. Install <a href="https://vulkan.lunarg.com/" target="_blank">`Vulkan SDK`</a>
2. Install <a href="https://github.com/Malience/EngineDevelopmentLibrary" target="_blank">`EDL`</a> or clone the repository into the ext directory
3. Build using CMAKE (build.bat file included for your convenience!)

## Required Dependencies
* <a href="https://vulkan.lunarg.com/" target="_blank">`Vulkan SDK`</a>
* <a href="https://cmake.org/download/" target="_blank">`CMAKE`</a>
* <a href="https://cmake.org/download/" target="_blank">`CMAKE`</a>
* Visual Studio 2022
* Updated Graphics Drivers

## Included Dependencies

* <a href="https://github.com/glfw/glfw" target="_blank">`GLFW`</a>
* <a href="https://github.com/g-truc/glm" target="_blank">`glm`</a>
* <a href="https://github.com/sheredom/json.h" target="_blank">`json.h`</a>
* <a href="https://github.com/guybrush77/rapidobj" target="_blank">`rapidobj`</a>
* <a href="https://github.com/nothings/stb" target="_blank">`stb`</a>