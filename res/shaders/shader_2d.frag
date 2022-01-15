#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 TexCoord;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D texture0;

void main() {
	fragColor = texture(texture0, TexCoord);
}