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
		std::cout << "OBJNAME: " << _renderer->GetGameObject()->Name << std::endl;
		//glm::vec3 velo = rigid_puck->GetVelocity();

		//std::cout << velo.x << " " << velo.y << " " << velo.z << std::endl;
		//std::cout << rigid_puck->GetAngularDamping() << std::endl;

		/*
		GameObject::Sptr edge = scene->FindObjectByGUID(edgeID[0]);
		glm::vec3 edgePos = edge->GetPosition();
		//edgePos = glm::normalize(edgePos);
		//velo = glm::normalize(velo);

		glm::vec3 dV = glm::normalize(velo - glm::vec3(0.0f, 0.0f, 0.0f));
		glm::vec3 dE = glm::normalize(edgePos - glm::vec3(0.0f, 0.0f, 0.0f));
		float rotAng = glm::acos(glm::dot(dV, dE));

		std::cout << rotAng << std::endl;
		*/
		glm::vec3 rigiVelo = rigidOBJ->GetVelocity();
		glm::vec3 edgeVec = trigger->GetGameObject()->GetPosition();
		float speed = glm::length(rigiVelo);

		rigiVelo = glm::normalize(rigiVelo);
		edgeVec = glm::normalize(edgeVec * -1.0f);

		glm::vec3 refVelo = glm::reflect(rigiVelo, edgeVec);
		refVelo.z = 0.0f;
		refVelo.x *= -1.0f;
		refVelo *= speed;

		rigidOBJ->resetVelocity();
		rigidOBJ->ApplyImpulse(refVelo);

		//std::cout << refVelo.x << " " << refVelo.y << " " << refVelo.z << std::endl;

		//rigidOBJ->SetAngularDamping(rotAng);
	}
}

void BounceBehaviour::OnLeavingTrigger(const Gameplay::Physics::TriggerVolume::Sptr& trigger) {
	LOG_INFO("Left trigger: {}", trigger->GetGameObject()->Name);
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
