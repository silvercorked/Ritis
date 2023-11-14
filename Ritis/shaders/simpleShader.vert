#version 450
// VERTEX SHADER

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform Push {
	mat4 transform;		// projection * view * model
	mat4 normalMatrix;	// actually a mat3, but mat4 for alignment
} push;

const vec3 DIRECTION_TO_LIGHT = normalize(vec3(1.0, -3.0, -1.0));
const float AMBIENT = 0.02;

void main() {
	// (-1, -1) ______ (1, -1)
	//          |    |
	//          |    |
	//          |____|
	//  (-1, 1)        (1, 1)
	// gl_Position = vec4(push.transform * position + push.offset, 0.0, 1.0); // x, y, z, w
	gl_Position = push.transform * vec4(position, 1.0); // 1 is homogenous coordinate. if position was direction vector, could use 0 to avoid affect of translation
	
	//vec3 normalWorldSpace = normalize(mat3(push.modelMatrix) * normal); // only works if uniform scaling (sx == sy == sz)

	//mat3 normalMatrix = transpose(inverse(mat3(push.modelMatrix))); // works for non-uniform scaling but slow
	//vec3 normalWorldSpace = normalize(normalMatrix * normal); // dont need last row or col, so chop off

	vec3 normalWorldSpace = normalize(mat3(push.normalMatrix) * normal); // faster by pre-computing inverse transpose on host

	float lightIntensity = AMBIENT + max(dot(normalWorldSpace, DIRECTION_TO_LIGHT), 0);
	
	fragColor = lightIntensity * color;
}
