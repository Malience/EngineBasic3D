#pragma once

#include "shaderc/shaderc.h"

#include <string>
#include <vector>

namespace edl {

enum class ShaderType : uint32_t {
	SHADER_TYPE_GLSL_VERTEX_SHADER = 0,
	SHADER_TYPE_GLSL_FRAGMENT_SHADER = 1,
	SHADER_TYPE_GLSL_GEOMETRY_SHADER = 2,
	SHADER_TYPE_GLSL_TESS_CONTROL_SHADER = 3,
	SHADER_TYPE_GLSL_TESS_EVALUATION_SHADER = 4,
	SHADER_TYPE_GLSL_TASK_SHADER = 4,
	SHADER_TYPE_GLSL_MESH_SHADER = 6,
	SHADER_TYPE_GLSL_COMPUTE_SHADER = 7,
	SHADER_TYPE_GLSL_RAYGEN_SHADER = 8,
	SHADER_TYPE_GLSL_MISS_SHADER = 9,
	SHADER_TYPE_GLSL_CLOSESTHIT_SHADER = 10,
	SHADER_TYPE_GLSL_ANYHIT_SHADER = 11,
	SHADER_TYPE_GLSL_INTERSECTION_SHADER = 12,
	SHADER_TYPE_NUM_SHADER_TYPES = 13,
};

struct shader_spv {
    std::string name;
    size_t size;
    std::vector<uint32_t> code;
};

class ShaderCompiler {
public:
    ShaderCompiler();
    virtual ~ShaderCompiler();

	shader_spv compileShader(const std::string& name, const ShaderType& shader_type, const std::vector<char>& code);

private:
    std::vector<char> buffer;

    shaderc_compiler_t compiler;
    shaderc_compile_options_t options;
};

}