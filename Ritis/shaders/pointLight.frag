#version 450

layout (location = 0) in vec2 fragOffset;
layout (location = 0) out vec4 outColor;

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
	float distance = sqrt(dot(fragOffset, fragOffset));
	if (distance >= 1.0) {
		discard; // fragment shader only keyword to ignore a fragment for rendering
	}
	outColor = vec4(push.color.xyz, 1.0);
}
