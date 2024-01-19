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
	mat4 inverseView;
	vec4 ambientLightColor; // w is intensity
	PointLight pointLights[10]; // could replace 10 with specialization constant at pipeline creation time
	int numLights;
} ubo;

layout(push_constant) uniform Push {
	mat4 modelMatrix;		// model
	mat4 normalMatrix;		// actually a mat3, but mat4 for alignment
} push;

void main() { // runs on per-fragment basis, so only on fragments generated from vertex shader
	vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w; // ambient contribution
	vec3 specularLight = vec3(0.0);
	vec3 surfaceNormal = normalize(fragNormalWorld);

	vec3 cameraPosWorld = ubo.inverseView[3].xyz;
	vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);

	for (int i = 0; i < ubo.numLights; i++) {
		PointLight light = ubo.pointLights[i];
		vec3 directionToLight = light.position.xyz - fragPosWorld;
		float attenuation = 1.0 / dot(directionToLight, directionToLight); // vec dotted by itself == the length of the vec squared
		directionToLight = normalize(directionToLight);

		// diffuse lighting
		float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0); // vertex normal . directionToLight * lightColor // vertex normal and dirToLight must be normalized
		vec3 lightIntensity = light.color.xyz * light.color.w * attenuation;
		diffuseLight += lightIntensity * cosAngIncidence;

		// specular lighting
		vec3 halfAngle = normalize(directionToLight + viewDirection);
		float blinnTerm = dot(surfaceNormal, halfAngle);
		blinnTerm = clamp(blinnTerm, 0, 1); // ignore when viewer is on opposite side of surface
		blinnTerm = pow(blinnTerm, 32.0);	// higher exponent -> sharper highlight. future parameter
		specularLight += lightIntensity * blinnTerm;
	}
	
	outColor = vec4(diffuseLight * fragColor + specularLight * fragColor, 1.0);
}