#version 450

const vec2 OFFSETS[6] = vec2[](
	vec2(-1.0, -1.0),
	vec2(-1.0, 1.0),
	vec2(1.0, -1.0),
	vec2(1.0, -1.0),
	vec2(-1.0, 1.0),
	vec2(1.0, 1.0)
);

layout (location = 0) out vec2 fragOffset;

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
	vec4 position;
	vec4 color;
	float radius;
} push;

void main() {
	fragOffset = OFFSETS[gl_VertexIndex];
	
	// computing light vertex positions in world space
	//vec3 cameraRightWorld = {ubo.view[0][0], ubo.view[1][0], ubo.view[2][0]};
	//vec3 cameraUpWorld = {ubo.view[0][1], ubo.view[1][1], ubo.view[2][1]};
	//vec3 positionWorld = push.position.xyz
	//	+ push.radius * fragOffset.x * cameraRightWorld
	//	+ push.radius * fragOffset.y * cameraUpWorld;
	//gl_Position = ubo.projection * ubo.view * vec4(positionWorld, 1.0);
	
	// computing light vertex positions in camera space
	vec4 lightInCameraSpace = ubo.view * push.position;
	vec4 positionInCameraSpace = lightInCameraSpace + (push.radius * vec4(fragOffset, 0.0, 0.0));
	gl_Position = ubo.projection * positionInCameraSpace;
}
