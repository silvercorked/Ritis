#version 450
// VERTEX SHADER

layout(location = 0) in vec2 position;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 fragColor;

void main() {
	// (-1, -1) ______ (1, -1)
	//          |    |
	//          |    |
	//          |____|
	//  (-1, 1)        (1, 1)
	gl_Position = vec4(position, 0.0, 1.0); // x, y, z, w
	fragColor = color;
}
