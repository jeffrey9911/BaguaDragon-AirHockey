#include "BounceBehaviour.h"
#include "Gameplay/Components/ComponentManager.h"
#include "Gameplay/GameObject.h"


BounceBehaviour::BounceBehaviour() :
	IComponent(),
	_renderer(nullptr),
	rigidOBJ(nullptr)
{ }
BounceBehaviour::~BounceBehaviour() = default;

void BounceBehaviour::OnEnteredTrigger(const Gameplay::Physics::TriggerVolume::Sptr& trigger) {
	if (_renderer && rigidOBJ) {
		glm::vec3 rigiVelo = rigidOBJ->GetVelocity();
		rigiVelo.z = 0.0f;

		glm::vec3 edgeVec;
		glm::vec3 forwVec = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::quat rot = trigger->GetGameObject()->GetRotation();
		edgeVec = glm::vec3(glm::vec4(forwVec, 1.0) * rot);
		edgeVec.x *= -1.0f;
		edgeVec.z = 0.0f;

		float speed = glm::length(rigiVelo);

		rigiVelo = glm::normalize(rigiVelo);
		edgeVec = glm::normalize(edgeVec);

		glm::vec3 refVelo = glm::reflect(rigiVelo, edgeVec);
		refVelo.z = 0.0f;

		refVelo *= speed * 0.5f;

		rigidOBJ->resetVelocity();
		rigidOBJ->ApplyImpulse(refVelo);
	}
}

void BounceBehaviour::OnLeavingTrigger(const Gameplay::Physics::TriggerVolume::Sptr& trigger) {

}



void BounceBehaviour::Awake() {
	_renderer = GetComponent<RenderComponent>();
}

void BounceBehaviour::RenderImGui() { }


nlohmann::json BounceBehaviour::ToJson() const {
	return {
		/*
		{ "enter_material", EnterMaterial != nullptr ? EnterMaterial->GetGUID().str() : "null" },
		{ "exit_material", ExitMaterial != nullptr ? ExitMaterial->GetGUID().str() : "null" }*/
	};
}

BounceBehaviour::Sptr BounceBehaviour::FromJson(const nlohmann::json& blob) {
	BounceBehaviour::Sptr result = std::make_shared<BounceBehaviour>();
	return result;
}
