#version 450
// FRAGMENT SHADER

layout (location = 0) in vec3 fragColor;

layout (location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
	mat4 modelMatrix;		// model
	mat4 normalMatrix;		// actually a mat3, but mat4 for alignment
} push;

void main() { // runs on per-fragment basis, so only on fragments generated from vertex shader
	outColor = vec4(fragColor, 1.0); // red, green, blue, alpha
}