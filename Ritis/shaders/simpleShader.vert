#version 450
// VERTEX SHADER

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;

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

void main() {
	// (-1, -1) ______ (1, -1)
	//          |    |
	//          |    |
	//          |____|
	//  (-1, 1)        (1, 1)
	// gl_Position = vec4(push.transform * position + push.offset, 0.0, 1.0); // x, y, z, w
	vec4 positionWorld = push.modelMatrix * vec4(position, 1.0); // vertex position in world space (from model space)
	gl_Position = ubo.projectionViewMatrix * positionWorld; // 1 is homogenous coordinate. if position was direction vector, could use 0 to avoid affect of translation

	vec3 normalWorldSpace = normalize(mat3(push.normalMatrix) * normal);

	vec3 directionToLight = ubo.lightPosition - positionWorld.xyz;
	float attenuation = 1.0 / dot(directionToLight, directionToLight); // vec dotted by itself == the length of the vec squared

	vec3 lightColor = ubo.lightColor.xyz * ubo.lightColor.w * attenuation;
	vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
	vec3 diffuseLight = lightColor * max(dot(normalWorldSpace, normalize(directionToLight)), 0); // vertex normal . directionToLight * lightColor // vertex normal and dirToLight must be normalized
	vec3 lightIntensity = diffuseLight + ambientLight;
	
	fragColor = lightIntensity * color;
}
