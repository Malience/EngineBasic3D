#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable

layout(location = 0) out vec4 position;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec3 tangent;
layout(location = 3) out vec3 bitangent;
layout(location = 4) out vec2 texCoord0;
layout(location = 5) out vec2 texCoord1;
layout(location = 6) out int materialOffset;
layout(location = 7) out mat3 TBN;

struct SceneData {
	vec4 cameraPosition;
	uint activeLights;
};

struct DrawData {
	int positionOffset;
	int normalOffset;
	int tangentOffset;
	//int bitangentOffset;
	
	int texCoord0Offset;
	//int texCoord1Offset;
	int materialOffset;
	
	int mvpOffset;
	//int invMVPOffset;
	
	uint padding0;
	uint padding1;
};

layout(std430, set = 0, binding = 0) buffer SceneDataBuffer {
	SceneData scene;
};

layout(std430, set = 1, binding = 0) buffer DrawDataBuffer {
	DrawData drawDatas[];
};

layout(std430, set = 2, binding = 0) buffer TransformBuffer {
	mat4 transforms[];
};

layout(std430, set = 3, binding = 0) buffer PositionBuffer {
	vec4 positions[];
};

layout(std430, set = 4, binding = 0) buffer NormalBuffer {
	vec4 normals[];
};

layout(std430, set = 5, binding = 0) buffer TexCoord0Buffer {
	vec2 texCoords[];
};


void main() {
	DrawData drawData = drawDatas[gl_DrawIDARB];

	position = positions[gl_VertexIndex + drawData.positionOffset];
	normal = normals[gl_VertexIndex + drawData.normalOffset].xyz;
	tangent = normals[gl_VertexIndex + drawData.tangentOffset].xyz;
	//bitangent = normals[gl_VertexIndex + drawData.bitangentOffset];
	texCoord0 = texCoords[gl_VertexIndex + drawData.texCoord0Offset];
	//texCoord1 = texCoords[gl_VertexIndex + drawData.texCoord1Offset];
	
	materialOffset = drawData.materialOffset;
	
	mat4 mvp = transforms[drawData.mvpOffset];
	gl_Position = mvp * position;
	
	//mat4 invMVP = transforms[drawData.invMVPOffset];
	
	normal = normalize((mvp * vec4(normal, 0.0)).xyz);
	tangent = normalize((mvp * vec4(tangent, 0.0)).xyz);
	tangent = normalize(tangent - dot(tangent, normal) * normal);
	bitangent = cross(normal, tangent);
	
	//bitangent = normalize((mvp * vec4(bitangent, 0.0)).xyz);
	
	TBN = mat3(tangent, bitangent, normal);
}
