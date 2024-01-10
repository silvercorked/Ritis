#version 450
// FRAGMENT SHADER

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragPosWorld;
layout (location = 2) in vec3 fragNormalWorld; // linear interpolation may have de-normalized

layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GlobalUbo {
	mat4 projectionViewMatrix;
	vec4 ambientLightColor; // w is intensity
	vec3 lightPosition;
	vec4 lightColor; // w is intensity
} ubo;

layout(push_constant) uniform Push {
	mat4 modelMatrix;		// model
	mat4 normalMatrix;		// actually a mat3, but mat4 for alignment
} push;

void main() { // runs on per-fragment basis, so only on fragments generated from vertex shader
	vec3 directionToLight = ubo.lightPosition - fragPosWorld;
	float attenuation = 1.0 / dot(directionToLight, directionToLight); // vec dotted by itself == the length of the vec squared

	vec3 lightColor = ubo.lightColor.xyz * ubo.lightColor.w * attenuation;
	vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
	vec3 diffuseLight = lightColor * max(dot(normalize(fragNormalWorld), normalize(directionToLight)), 0); // vertex normal . directionToLight * lightColor // vertex normal and dirToLight must be normalized
	vec3 lightIntensity = diffuseLight + ambientLight;
	
	outColor = vec4(lightIntensity * fragColor, 1.0);
}