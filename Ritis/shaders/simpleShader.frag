#version 450
// FRAGMENT SHADER

layout (location = 0) in vec3 fragColor;

layout (location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
	mat4 transform;		// projection * view * model
	mat4 normalMatrix;	
} push;

void main() { // runs on per-fragment basis, so only on fragments generated from vertex shader
	outColor = vec4(fragColor, 1.0); // red, green, blue, alpha
}