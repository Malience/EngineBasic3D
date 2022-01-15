#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 normal;
layout(location = 2) in vec2 texCoord0;
layout(location = 3) in vec2 texCoord1;

layout(location = 4) flat in uint materialOffset;

layout(location = 0) out vec4 fragColor;

layout(set = 6, binding = 0) uniform sampler2D textures[];

void main() {
	fragColor = texture(textures[materialOffset], texCoord0);

	if(fragColor.a < 1.0f) {
		discard;
	}
}