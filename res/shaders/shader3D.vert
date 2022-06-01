#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_buffer_reference2 : enable

layout (location = 0) out TriangleData {
    uint64_t material;
};

layout (location = 2) out VertexData {
    vec4 position;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    vec2 texCoord;
    mat3 TBN;
};

layout(buffer_reference, std430, buffer_reference_align = 16) buffer Vec4Ptr { 
    vec4 v;
};

layout(buffer_reference, std430, buffer_reference_align = 8) buffer Vec2Ptr { 
    vec2 v;
};

layout(buffer_reference, std430, buffer_reference_align = 16) buffer Mat4Ptr { 
    mat4 m;
};

layout(buffer_reference, std430, buffer_reference_align = 4) buffer UintPtr { 
    uint v;
};

struct MaterialSet {
    uint64_t materials[16];
};

layout(buffer_reference, std430, buffer_reference_align = 16) buffer MaterialSetPtr { 
    MaterialSet m;
};

struct Meshlet {
    uint vertCount;
    uint triCount;

    Vec4Ptr positions;
    Vec4Ptr normals;
    Vec4Ptr tangents;
    Vec2Ptr texcoords;

    UintPtr indices;
    UintPtr materials;
    
    uint64_t padding0;
};

layout(buffer_reference, std430, buffer_reference_align = 16) buffer MeshletPtr { 
    Meshlet m;
};

struct Mesh {
    uint meshletCount;
    uint padding0;
    MeshletPtr meshlets;
};

layout(buffer_reference, std430, buffer_reference_align = 16) buffer MeshPtr { 
    Mesh m;
};

struct SceneData {
	vec4 cameraPosition;
	
	vec4 lightDir;
	vec4 lightColor;
	
	float directionalLightPower;
	uint activeLights;
	int pad0;
	int pad1;
};

struct DrawCommand {
    MeshPtr mesh;
    uint64_t meshlet;
	MaterialSetPtr materials;
	Mat4Ptr mvp;
};

layout(buffer_reference, std430, buffer_reference_align = 16) buffer ScenePtr { 
    SceneData scene;
};

layout(buffer_reference, std430, buffer_reference_align = 16) buffer DrawPtr { 
    DrawCommand data;
};

layout(push_constant) uniform fragmentPushConstants {
    layout(offset = 0) ScenePtr scenePtr;
    layout(offset = 8) DrawPtr commandsPtr;
};

void main() {
    uint id = gl_VertexIndex;
    uint command_id = gl_DrawIDARB;
    
    SceneData scene = scenePtr.scene;
    DrawCommand command = (commandsPtr + command_id).data;
	//DrawData drawData = (drawDatas + gl_DrawIDARB).data;

    Mesh mesh = command.mesh.m;
    Meshlet meshlet = (mesh.meshlets + command.meshlet).m;
    MaterialSet materialSet = command.materials.m;
    mat4 mvp = command.mvp.m;
    
    Vec4Ptr positions = meshlet.positions;
    Vec4Ptr normals = meshlet.normals;
    Vec4Ptr tangents = meshlet.tangents;
    Vec2Ptr texcoords = meshlet.texcoords;
    
    
    position = mvp * (positions + id).v;
    normal = normalize((mvp * (normals + id).v).xyz);
    
    tangent = (tangents + id).v.xyz;
    texCoord = (texcoords + id).v;
    
    position = position;
    normal = normal;
    tangent = normalize((mvp * vec4(tangent, 0.0)).xyz);
    tangent = normalize(tangent - dot(tangent, normal) * normal);
    bitangent = cross(normal, tangent);
    
    TBN = mat3(tangent, bitangent, normal);
    
    
    texCoord.y = 1.0 - texCoord.y;
    
    gl_Position = position;
    
    UintPtr materials = meshlet.materials;
    
    uint triID = uint(floor(id / 3));
    uint mat = (materials + triID).v;
    material = materialSet.materials[0]; // CHANGE THIS

}
