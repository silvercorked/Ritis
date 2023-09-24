#version 450
// FRAGMENT SHADER

layout (location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
	mat 2 transform;
	vec2 offset;
	vec3 color;
} push;

void main() { // runs on per-fragment basis, so only on fragments generated from vertex shader
	outColor = vec4(push.color, 1.0); // red, green, blue, alpha
}