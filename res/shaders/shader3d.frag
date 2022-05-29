#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_buffer_reference2 : enable

layout (location = 0) in TriangleData {
    flat uint64_t materialOffset;
};

layout (location = 2) in VertexData {
    vec4 position;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    vec2 texCoord0;
    mat3 TBN;
};

layout(location = 0) out vec4 fragColor;

struct SceneData {
	vec4 cameraPosition;
	
	vec4 lightDir;
	vec4 lightColor;
	
	float directionalLightPower;
	uint activeLights;
	int pad0;
	int pad1;
};

struct PBRMaterial {
    vec4 tint;

    float metallic;
    float roughness;
    float ao;

    int albedoTexture;
    int normalTexture;
    //int roughnessTexture;
    int pad0;
    int pad1;
    int pad2;
    
    int pad3;
    int pad4;
    int pad5;
    int pad6;
};

struct Light {
	vec4 position;
	vec4 direction;
	vec4 color;
	
	uint type;
	uint pad0;
	uint pad1;
	uint pad2;
};

layout(buffer_reference, std430, buffer_reference_align = 16) buffer MaterialPtr { 
    PBRMaterial material;
};

layout(buffer_reference, std430, buffer_reference_align = 16) buffer ScenePtr { 
    SceneData scene;
};

layout(buffer_reference, std430, buffer_reference_align = 16) buffer LightPtr { 
    Light light;
};

layout(push_constant) uniform fragmentPushConstants {
    layout(offset = 0) ScenePtr sceneData;
    layout(offset = 16) LightPtr lights;
};

layout(set = 0, binding = 0) uniform sampler2D textures[];

const float PI = 3.14159265359;
  
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

vec3 CalculateDirectionalLight(vec3 N, vec3 V, vec3 F0, vec3 albedo, float metallic, float roughness) {
	vec3 L = normalize(sceneData.scene.lightDir.xyz);
	vec3 H = normalize(V + L);
	vec3 radiance = sceneData.scene.lightColor.rgb * sceneData.scene.directionalLightPower;
	
	// cook-torrance brdf
	float NDF = DistributionGGX(N, H, roughness);        
	float G   = GeometrySmith(N, V, L, roughness);      
	vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
	
	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metallic;
	
	vec3 numerator = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
	vec3 specular = numerator / denominator;
	
	float NdotL = max(dot(N, L), 0.0);
	return (kD * albedo.rgb / PI + specular) * radiance * NdotL;
}

void main() {

	PBRMaterial material = MaterialPtr(materialOffset).material;//materials[materialOffset];
	//PBRMaterial material = materials[materialOffset];
	
	vec4 albedoValue = texture(textures[material.albedoTexture], texCoord0);
	
	if(albedoValue.a < 1.0f) {
		discard;
	}
	
	//vec4 roughnessTexture = texture(textures[material.roughnessTexture], texCoord0);
	
	
	float metallic = material.metallic;
	float roughness = material.roughness;
	float ao = material.ao;
	
	vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedoValue.rgb, metallic);
	
	//vec4 normalValue; //Calculate with normalTexture and normal and blah blah blah
	
	vec3 N;
	if(material.normalTexture >= 0) {
		vec3 normalValue = texture(textures[material.normalTexture], texCoord0).xyz;
		normalValue = normalValue * 2.0 - 1.0;  
		N = normalize(TBN * normalValue);
	}
	else {
		N = normalize(normal);
	}
	
    vec3 V = normalize(sceneData.scene.cameraPosition.xyz - position.xyz);
	
	vec3 Lo = vec3(0);
	Lo += CalculateDirectionalLight(N, V, F0, albedoValue.rgb, metallic, roughness);
	for(int i = 0; i < sceneData.scene.activeLights; i++) {
		Light light = (lights + i).light;
		
		vec3 L = normalize(light.position.xyz - position.xyz);
		vec3 H = normalize(V + L);
		float distance = length(light.position.xyz - position.xyz);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance = light.color.rgb * attenuation;
		
		// cook-torrance brdf
        float NDF = DistributionGGX(N, H, roughness);        
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
		
		vec3 kS = F;
		vec3 kD = vec3(1.0) - kS;
		kD *= 1.0 - metallic;
		
		vec3 numerator = NDF * G * F;
		float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
		vec3 specular = numerator / denominator;
		
		float NdotL = max(dot(N, L), 0.0);
		Lo += (kD * albedoValue.rgb / PI + specular) * radiance * NdotL;
	}
	
	vec3 ambient = vec3(0.03) * albedoValue.rgb * ao;
	vec3 color = ambient + Lo;
	
	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0/2.2));
	
	fragColor = vec4(color, albedoValue.a);
	//fragColor = vec4(1.0f, 1.0f, 0.0f, 1.0f);
}