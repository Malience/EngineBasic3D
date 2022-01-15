#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable

layout(location = 0) out vec4 position;
layout(location = 1) out vec4 normal;
layout(location = 2) out vec2 texCoord0;
layout(location = 3) out vec2 texCoord1;

layout(location = 4) out uint materialOffset;

struct DrawData {
	uint positionOffset;
	uint normalOffset;
	uint texCoord0Offset;
	uint texCoord1Offset;
	
	uint materialOffset;
	uint transformOffset;
	
	uint padding0;
	uint padding1;
};

layout(set = 0, binding = 0) buffer DrawDataBuffer {
	DrawData drawDatas[];
};

layout(std430, set = 1, binding = 0) buffer TransformBuffer {
	mat4 transforms[];
};

layout(std430, set = 2, binding = 0) buffer PositionBuffer {
	vec4 positions[];
};

layout(std430, set = 3, binding = 0) buffer NormalBuffer {
	vec4 normals[];
};

layout(std430, set = 4, binding = 0) buffer TexCoord0Buffer {
	vec2 texCoords0[];
};

layout(std430, set = 5, binding = 0) buffer TexCoord1Buffer {
	vec2 texCoords1[];
};


void main() {
	DrawData drawData = drawDatas[gl_DrawIDARB];

	position = positions[gl_VertexIndex + drawData.positionOffset];
	normal = normals[gl_VertexIndex + drawData.normalOffset];
	texCoord0 = texCoords0[gl_VertexIndex + drawData.texCoord0Offset];
	texCoord1 = texCoords1[gl_VertexIndex + drawData.texCoord1Offset];
	
	materialOffset = drawData.materialOffset;
	
	mat4 mvp = transforms[drawData.transformOffset];
	gl_Position = mvp * position;
}
