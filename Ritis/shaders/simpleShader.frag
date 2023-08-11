#version 450
// FRAGMENT SHADER

layout (location = 0) out vec4 outColor;

void main() { // runs on per-fragment basis, so only on fragments generated from vertex shader
	outColor = vec4(1.0, 0.0, 0.0, 1.0); // red, green, blue, alpha
}