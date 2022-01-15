#version 450
#extension GL_ARB_separate_shader_objects : enable

vec2 positions[6] = vec2[](
    vec2(0.0f, 0.0f),
    vec2(1.0f, 0.0f),
    vec2(1.0f, 1.0f),
    vec2(0.0f, 0.0f),
    vec2(1.0f, 1.0f),
    vec2(0.0f, 1.0f)
);

layout(location = 0) out vec2 TexCoord;

layout(binding = 0) uniform UniformBufferObject {
	mat3 model;
};

void main() {
	TexCoord = positions[gl_VertexIndex];
	gl_Position = vec4(TexCoord, 0.0f, 1.0f);//vec4((model * vec3(TexCoord, 1.0)).xy, 0.0f, 1.0f);
}