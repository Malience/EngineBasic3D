#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec3 bitangent;
layout(location = 4) in vec2 texCoord0;
layout(location = 5) in vec2 texCoord1;
layout(location = 6) flat in int materialOffset;
layout(location = 7) in mat3 TBN;

layout(location = 0) out vec4 fragColor;

struct SceneData {
	vec4 cameraPosition;
	uint activeLights;
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
    //int pad3;
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

layout(std430, set = 0, binding = 0) buffer SceneDataBuffer {
	SceneData scene;
};

layout(std430, set = 6, binding = 0) buffer MaterialBuffer {
	PBRMaterial materials[];
};

layout(std430, set = 7, binding = 0) buffer LightBuffer {
	Light lights[];
};

layout(set = 8, binding = 0) uniform sampler2D textures[];

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

void main() {
	PBRMaterial material = materials[materialOffset];
	
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
	
    vec3 V = normalize(scene.cameraPosition.xyz - position.xyz);
	
	vec3 Lo = vec3(0.0);
	for(int i = 0; i < scene.activeLights; i++) {
		Light light = lights[i];
		
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
}