#version 450
// FRAGMENT SHADER

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragPosWorld;
layout (location = 2) in vec3 fragNormalWorld; // linear interpolation may have de-normalized

layout (location = 0) out vec4 outColor;

struct PointLight {
	vec4 position; // ignore w
	vec4 color; // w is intensity
};

layout(set = 0, binding = 0) uniform GlobalUbo {
	mat4 projection;
	mat4 view;
	vec4 ambientLightColor; // w is intensity
	PointLight pointLights[10]; // could replace 10 with specialization constant at pipeline creation time
	int numLights;
} ubo;

layout(push_constant) uniform Push {
	mat4 modelMatrix;		// model
	mat4 normalMatrix;		// actually a mat3, but mat4 for alignment
} push;

void main() { // runs on per-fragment basis, so only on fragments generated from vertex shader
	vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
	vec3 surfaceNormal = normalize(fragNormalWorld);

	for (int i = 0; i < ubo.numLights; i++) {
		PointLight light = ubo.pointLights[i];
		vec3 directionToLight = light.position.xyz - fragPosWorld;
		float attenuation = 1.0 / dot(directionToLight, directionToLight); // vec dotted by itself == the length of the vec squared
		float cosAngIncidence = max(dot(surfaceNormal, normalize(directionToLight)), 0); // vertex normal . directionToLight * lightColor // vertex normal and dirToLight must be normalized
		vec3 lightIntensity = light.color.xyz * light.color.w * attenuation;
		diffuseLight += lightIntensity * cosAngIncidence;
	}
	
	outColor = vec4(diffuseLight * fragColor, 1.0);
}