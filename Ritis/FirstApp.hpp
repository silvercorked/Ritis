#pragma once

#include "Window.hpp"
#include "GameObject.hpp"
#include "Renderer.hpp"
#include "SimpleRenderSystem.hpp"

#define GLM_FORCE_RADIANS					// functions expect radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE			// Depth buffer values will range from 0 to 1, not -1 to 1
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <memory>
#include <vector>
#include <stdexcept>
#include <array>

namespace engine {

	class GravityPhysicsSystem {
	public:
		GravityPhysicsSystem(float strength) : strengthGravity{ strength } {}

		const float strengthGravity;

		auto update(std::vector<GameObject>& objs, float dt, unsigned int substeps = 1) -> void {
			const float stepDelta = dt / substeps;
			for (int i = 0; i < substeps; i++)
				stepSimulation(objs, stepDelta);
		}

		auto computeForce(GameObject& fromObj, GameObject& toObj) const -> glm::vec2 {
			auto offset = fromObj.transform2d.translation - toObj.transform2d.translation;
			float distanceSquared = glm::dot(offset, offset);
			if (glm::abs(distanceSquared) < 1e-10f) return { 0.0f, 0.0f }; // this is not true, but fine for a sim
			float force = (strengthGravity * toObj.rigidBody2d.mass * fromObj.rigidBody2d.mass) / distanceSquared;
			return (force * offset) / glm::sqrt(distanceSquared);
		}
	private:
		auto stepSimulation(std::vector<GameObject>& physicsObjs, float dt) -> void {
			for (auto iterA = physicsObjs.begin(); iterA != physicsObjs.end(); iterA++) {
				auto& objA = *iterA;
				for (auto iterB = iterA + 1; iterB != physicsObjs.end(); iterB++) {
					auto& objB = *iterB;
					auto force = computeForce(objA, objB);
					objA.rigidBody2d.velocity += dt * -force / objA.rigidBody2d.mass;
					objB.rigidBody2d.velocity += dt * force / objB.rigidBody2d.mass;
				}
			}
			for (auto& obj : physicsObjs) {
				obj.transform2d.translation += dt * obj.rigidBody2d.velocity;
			}
		}
	};
	class Vec2FieldSystem {
	public:
		auto update(
			const GravityPhysicsSystem& physicsSystem,
			std::vector<GameObject>& PhysicsObjs,
			std::vector<GameObject>& vectorField
		) {
			for (auto& vf : vectorField) {
				glm::vec2 direction{};
				for (auto& obj : PhysicsObjs) {
					direction += physicsSystem.computeForce(obj, vf);
				}
				vf.transform2d.scale.x = 0.005f + 0.045f * glm::clamp(glm::log(glm::length(direction) + 1) / 3.0f, 0.f, 1.f);
				vf.transform2d.rotation = atan2(direction.y, direction.x);
			}
		}
	};

	std::unique_ptr<Model> createSquareModel(Device& device, glm::vec2 offset) {
		std::vector<Model::Vertex> vertices = {
			{{-0.5f, -0.5f}},
			{{0.5f, 0.5f}},
			{{-0.5f, 0.5f}},
			{{-0.5f, -0.5f}},
			{{0.5f, -0.5f}},
			{{0.5f, 0.5f}}
		};
		for (auto& v : vertices) {
			v.position += offset;
		}
		return std::make_unique<Model>(device, vertices);
	}
	std::unique_ptr<Model> createCircleModel(Device& device, unsigned int numSides) {
		std::vector<Model::Vertex> uniqueVertices{};
		for (int i = 0; i < numSides; i++) {
			float angle = i * glm::two_pi<float>() / numSides;
			uniqueVertices.push_back({ {glm::cos(angle), glm::sin(angle)} });
		}
		uniqueVertices.push_back({}); // add vertex at 0,0
		std::vector<Model::Vertex> verticies{};
		for (int i = 0; i < numSides; i++) {
			verticies.push_back(uniqueVertices[i]);
			verticies.push_back(uniqueVertices[i + 1 % numSides]);
			verticies.push_back(uniqueVertices[numSides]); // 0,0
		}
		return std::make_unique<Model>(device, verticies);
	}
	auto nextSierpinski(const std::vector<Model::Vertex>& prev) {
		std::vector<Model::Vertex> next = {};
		for (auto i = 0; i < prev.size(); i += 3) { // prev[i][i + 1] and [i + 2] hold the triangl's verticies
			glm::vec2 avg0_1 = (prev[i].position + prev[i + 1].position) / 2.0f; // mid-point between vertex 0 and 1
			glm::vec2 avg0_2 = (prev[i].position + prev[i + 2].position) / 2.0f; // mid-point between vertex 0 and 2
			glm::vec2 avg1_2 = (prev[i + 1].position + prev[i + 2].position) / 2.0f; // mid-point between vertex 1 and 2
			{	// first triangle (0th vertex + 01 midpoint + 02 midpoint)
				next.push_back({ prev[i].position , prev[i].color });
				next.push_back({ avg0_1, prev[i + 1].color });
				next.push_back({ avg0_2, prev[i + 2].color });
			}
			{	// second (1st vertex + 01 midpoint + 12 midpoint)
				next.push_back({ prev[i + 1].position , prev[i + 1].color });
				next.push_back({ avg0_1, prev[i].color });
				next.push_back({ avg1_2, prev[i + 2].color });
			}
			{	// third (2nd vertex + 02 midpoint + 12 midpoint)
				next.push_back({ prev[i + 2].position , prev[i + 2].color });
				next.push_back({ avg0_2, prev[i].color });
				next.push_back({ avg1_2, prev[i + 1].color });
			}
		}
		return next;
	}

	class FirstApp {
		Window window{ WIDTH, HEIGHT, "Hello, World!" };
		Device device{ window };
		Renderer renderer{ window, device };

		std::vector<GameObject> gameObjects;

		auto loadGameObjects() -> void;
	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;
		
		FirstApp();
		~FirstApp();

		FirstApp(const FirstApp&) = delete;
		FirstApp& operator=(const FirstApp&) = delete;

		auto run() -> void;
	};

	FirstApp::FirstApp() {
		this->loadGameObjects();
	}
	FirstApp::~FirstApp() {}

	auto FirstApp::loadGameObjects() -> void {
		std::vector<Model::Vertex> verticies {
			{ {0.0f, -0.5f}, { 1.0f, 0.0f, 0.0f } },
			{ {0.5f, 0.5f}, {0.0f, 1.0f, 0.0f} },
			{ {-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f} }
		};
		//const auto SeirpinskiDepth = 2;
		//for (auto i = 0; i < SeirpinskiDepth; i++)
		//	verticies = nextSierpinski(verticies);

		auto model = std::make_shared<Model>(this->device, verticies);

		auto triangle = GameObject::createGameObject();
		triangle.model = model;
		triangle.color = { 0.1f, 0.8f, 0.1f };
		triangle.transform2d.translation.x = 0.2f;
		triangle.transform2d.scale = { 0.2f, 0.5f };
		triangle.transform2d.rotation = 0.25f * glm::two_pi<float>();

		this->gameObjects.push_back(std::move(triangle));
	}
	auto FirstApp::run() -> void {
		std::shared_ptr<Model> squareModel = createSquareModel(
			this->device, { 0.5f, 0.0f } // offset model by 0.5 so rotation occurs at edge rather than center of square
		);
		std::shared_ptr<Model> circleModel = createCircleModel(this->device, 64);

		std::vector<GameObject> physicsObjects{};
		auto red = GameObject::createGameObject();
		red.transform2d.scale = glm::vec2{ 0.05f };
		red.transform2d.translation = { 0.5f, 0.5f };
		red.color = { 1.0f, 0.0f, 0.0f };
		red.rigidBody2d.velocity = { -0.5f, 0.0f };
		red.model = circleModel;
		physicsObjects.push_back(std::move(red));
		auto blue = GameObject::createGameObject();
		blue.transform2d.scale = glm::vec2{ 0.05f };
		blue.transform2d.translation = { -0.45f, -0.25f };
		blue.color = { 0.0f, 0.0f, 1.0f };
		blue.rigidBody2d.velocity = { 0.5f, 0.0f };
		blue.model = circleModel;
		physicsObjects.push_back(std::move(blue));
		/*auto green = GameObject::createGameObject();
		green.transform2d.scale = glm::vec2{ 0.05f };
		green.transform2d.translation = { 0.0f, -0.1f };
		green.color = { 0.0f, 1.0f, 0.0f };
		green.rigidBody2d.velocity = { 0.0f, 0.02f };
		green.model = circleModel;
		physicsObjects.push_back(std::move(green));
		*/
		std::vector<GameObject> vectorField{};
		int gridCount = 40;
		for (int i = 0; i < gridCount; i++) {
			for (int j = 0; j < gridCount; j++) {
				auto vf = GameObject::createGameObject();
				vf.transform2d.scale = glm::vec2(0.005f);
				vf.transform2d.translation = {
					-1.0f + (i + 0.5) * 2.0f / gridCount,
					-1.0f + (j + 0.5) * 2.0f / gridCount
				};
				vf.color = glm::vec3{ 1.0f };
				vf.model = squareModel;
				vectorField.push_back(std::move(vf));
			}
		}

		GravityPhysicsSystem gravitySystem{ 0.81f };
		Vec2FieldSystem vecFieldSystem{};

		SimpleRenderSystem simpleRenderSystem{ this->device, this->renderer.getSwapChainRenderPass() };
		while (!this->window.shouldClose()) {
			glfwPollEvents();
			if (auto commandBuffer = this->renderer.beginFrame()) {
				gravitySystem.update(physicsObjects, 1.0f / 60, 1);
				vecFieldSystem.update(gravitySystem, physicsObjects, vectorField);

				this->renderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(commandBuffer, physicsObjects);
				simpleRenderSystem.renderGameObjects(commandBuffer, vectorField);
				this->renderer.endSwapChainRenderPass(commandBuffer);
				this->renderer.endFrame();
			}
		}
		vkDeviceWaitIdle(this->device.device());
	}
}
