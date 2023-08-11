#version 450
// VERTEX SHADER

vec2 positions[3] = vec2[] (
	vec2(0.0, -0.5),
	vec2(0.5, 0.5),
	vec2(-0.5, -0.5)
);

void main() {
	// (-1, -1) ______ (1, -1)
	//          |    |
	//          |    |
	//          |____|
	//  (-1, 1)        (1, 1)
	gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0); // x, y, z, w
}
