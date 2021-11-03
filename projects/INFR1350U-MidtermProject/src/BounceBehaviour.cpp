#include "BounceBehaviour.h"
#include "Gameplay/Components/ComponentManager.h"
#include "Gameplay/GameObject.h"

BounceBehaviour::BounceBehaviour() :
	IComponent(),
	_renderer(nullptr)
{ }
BounceBehaviour::~BounceBehaviour() = default;

void BounceBehaviour::OnEnteredTrigger(const Gameplay::Physics::TriggerVolume::Sptr& trigger) {
	if (_renderer) {
		std::cout << "OBJNAME: " << _renderer->GetGameObject()->Name << std::endl;
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
