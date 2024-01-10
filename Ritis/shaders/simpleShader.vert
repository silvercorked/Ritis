#version 450
// VERTEX SHADER

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;

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

	fragNormalWorld = normalize(mat3(push.normalMatrix) * normal);
	fragPosWorld = positionWorld.xyz;
	fragColor = color;
}
