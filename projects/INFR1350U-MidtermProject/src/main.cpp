#include <Logging.h>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <json.hpp>
#include <fstream>
#include <sstream>
#include <typeindex>
#include <optional>
#include <string>

// GLM math library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/common.hpp> // for fmod (floating modulus)

// Graphics
#include "Graphics/IndexBuffer.h"
#include "Graphics/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/Shader.h"
#include "Graphics/Texture2D.h"
#include "Graphics/VertexTypes.h"

// Utilities
#include "Utils/MeshBuilder.h"
#include "Utils/MeshFactory.h"
#include "Utils/ObjLoader.h"
#include "Utils/ImGuiHelper.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/FileHelpers.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/StringUtils.h"
#include "Utils/GlmDefines.h"

// Gameplay
#include "Gameplay/Material.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"

// Components
#include "Gameplay/Components/IComponent.h"
#include "Gameplay/Components/Camera.h"
#include "Gameplay/Components/RotatingBehaviour.h"
#include "Gameplay/Components/JumpBehaviour.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Gameplay/Components/MaterialSwapBehaviour.h"

// Physics
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/Colliders/BoxCollider.h"
#include "Gameplay/Physics/Colliders/PlaneCollider.h"
#include "Gameplay/Physics/Colliders/SphereCollider.h"
#include "Gameplay/Physics/Colliders/ConvexMeshCollider.h"
#include "Gameplay/Physics/TriggerVolume.h"
//#include "../../Sandbox/src/Graphics/DebugDraw.h"

// BounceBehaviour
#include "BounceBehaviour.h"

//#define LOG_GL_NOTIFICATIONS

/*
	Handles debug messages from OpenGL
	https://www.khronos.org/opengl/wiki/Debug_Output#Message_Components
	@param source    Which part of OpenGL dispatched the message
	@param type      The type of message (ex: error, performance issues, deprecated behavior)
	@param id        The ID of the error or message (to distinguish between different types of errors, like nullref or index out of range)
	@param severity  The severity of the message (from High to Notification)
	@param length    The length of the message
	@param message   The human readable message from OpenGL
	@param userParam The pointer we set with glDebugMessageCallback (should be the game pointer)
*/
void GlDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	std::string sourceTxt;
	switch (source) {
		case GL_DEBUG_SOURCE_API: sourceTxt = "DEBUG"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM: sourceTxt = "WINDOW"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceTxt = "SHADER"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY: sourceTxt = "THIRD PARTY"; break;
		case GL_DEBUG_SOURCE_APPLICATION: sourceTxt = "APP"; break;
		case GL_DEBUG_SOURCE_OTHER: default: sourceTxt = "OTHER"; break;
	}
	switch (severity) {
		case GL_DEBUG_SEVERITY_LOW:          LOG_INFO("[{}] {}", sourceTxt, message); break;
		case GL_DEBUG_SEVERITY_MEDIUM:       LOG_WARN("[{}] {}", sourceTxt, message); break;
		case GL_DEBUG_SEVERITY_HIGH:         LOG_ERROR("[{}] {}", sourceTxt, message); break;
			#ifdef LOG_GL_NOTIFICATIONS
		case GL_DEBUG_SEVERITY_NOTIFICATION: LOG_INFO("[{}] {}", sourceTxt, message); break;
			#endif
		default: break;
	}
}

//// Window Setup ////
#pragma region GeneralSetUp
// Stores our GLFW window in a global variable for now
GLFWwindow* window;
// The current size of our window in pixels
glm::ivec2 windowSize = glm::ivec2(1920, 1080);
// The title of our GLFW window
std::string windowTitle = "INFR1350U-Midterm-Airhockey-Jeffrey&Justin";


// using namespace should generally be avoided, and if used, make sure it's ONLY in cpp files
using namespace Gameplay;
using namespace Gameplay::Physics;

// The scene that we will be rendering
Scene::Sptr scene = nullptr;

void GlfwWindowResizedCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	windowSize = glm::ivec2(width, height);
	if (windowSize.x * windowSize.y > 0) {
		scene->MainCamera->ResizeWindow(width, height);
	}
}

/// <summary>
/// Handles intializing GLFW, should be called before initGLAD, but after Logger::Init()
/// Also handles creating the GLFW window
/// </summary>
/// <returns>True if GLFW was initialized, false if otherwise</returns>
bool initGLFW() {
	// Initialize GLFW
	if (glfwInit() == GLFW_FALSE) {
		LOG_ERROR("Failed to initialize GLFW");
		return false;
	}

	//Create a new GLFW window and make it current
	window = glfwCreateWindow(windowSize.x, windowSize.y, windowTitle.c_str(), nullptr, nullptr);
	glfwMakeContextCurrent(window);
	
	// Set our window resized callback
	glfwSetWindowSizeCallback(window, GlfwWindowResizedCallback);

	return true;
}

/// <summary>
/// Handles initializing GLAD and preparing our GLFW window for OpenGL calls
/// </summary>
/// <returns>True if GLAD is loaded, false if there was an error</returns>
bool initGLAD() {
	if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0) {
		LOG_ERROR("Failed to initialize Glad");
		return false;
	}
	return true;
}
#pragma endregion

/// <summary>
/// Draws a widget for saving or loading our scene
/// </summary>
/// <param name="scene">Reference to scene pointer</param>
/// <param name="path">Reference to path string storage</param>
/// <returns>True if a new scene has been loaded</returns>
bool DrawSaveLoadImGui(Scene::Sptr& scene, std::string& path) {
	// Since we can change the internal capacity of an std::string,
	// we can do cool things like this!
	ImGui::InputText("Path", path.data(), path.capacity());

	// Draw a save button, and save when pressed
	if (ImGui::Button("Save")) {
		scene->Save(path);
	}
	ImGui::SameLine();
	// Load scene from file button
	if (ImGui::Button("Load")) {
		// Since it's a reference to a ptr, this will
		// overwrite the existing scene!
		scene = nullptr;
		scene = Scene::Load(path);

		return true;
	}
	return false;
}

/// <summary>
/// Draws some ImGui controls for the given light
/// </summary>
/// <param name="title">The title for the light's header</param>
/// <param name="light">The light to modify</param>
/// <returns>True if the parameters have changed, false if otherwise</returns>
bool DrawLightImGui(const Scene::Sptr& scene, const char* title, int ix) {
	bool isEdited = false;
	bool result = false;
	Light& light = scene->Lights[ix];
	ImGui::PushID(&light); // We can also use pointers as numbers for unique IDs
	if (ImGui::CollapsingHeader(title)) {
		isEdited |= ImGui::DragFloat3("Pos", &light.Position.x, 0.01f);
		isEdited |= ImGui::ColorEdit3("Col", &light.Color.r);
		isEdited |= ImGui::DragFloat("Range", &light.Range, 0.1f);

		result = ImGui::Button("Delete");
	}
	if (isEdited) {
		scene->SetShaderLight(ix);
	}

	ImGui::PopID();
	return result;
}

double f0_cursorPosX, f0_cursorPosY;

int main() {
//// Initialize ////
	#pragma region GeneralInitialize
	Logger::Init(); // We'll borrow the logger from the toolkit, but we need to initialize it

	//Initialize GLFW
	if (!initGLFW())
		return 1;

	//Initialize GLAD
	if (!initGLAD())
		return 1;

	// Let OpenGL know that we want debug output, and route it to our handler function
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(GlDebugMessage, nullptr);

	// Initialize our ImGui helper
	ImGuiHelper::Init(window);

	// Initialize our resource manager
	ResourceManager::Init();

	// Register all our resource types so we can load them from manifest files
	ResourceManager::RegisterType<Texture2D>();
	ResourceManager::RegisterType<Material>();
	ResourceManager::RegisterType<MeshResource>();
	ResourceManager::RegisterType<Shader>();

	// Register all of our component types so we can load them from files
	ComponentManager::RegisterType<Camera>();
	ComponentManager::RegisterType<RenderComponent>();
	ComponentManager::RegisterType<RigidBody>();
	ComponentManager::RegisterType<TriggerVolume>();
	ComponentManager::RegisterType<RotatingBehaviour>();
	ComponentManager::RegisterType<JumpBehaviour>();
	ComponentManager::RegisterType<MaterialSwapBehaviour>();
	ComponentManager::RegisterType<BounceBehaviour>();
	#pragma endregion

	// GL states, we'll enable depth testing and backface fulling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

	bool loadScene = false;
	// For now we can use a toggle to generate our scene vs load from file
	if (loadScene) {
		ResourceManager::LoadManifest("manifest.json");
		scene = Scene::Load("scene.json");
	} 
	else {

//// Resources ////
		#pragma region OpenGL Resources Loading
		
		//// Shaders
		Shader::Sptr uboShader = ResourceManager::CreateAsset<Shader>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shader.glsl" }, 
			{ ShaderPartType::Fragment, "shaders/frag_blinn_phong_textured.glsl" }
		}); 

		//// Monkey & Box (will be removed later)
		MeshResource::Sptr monkeyMesh = ResourceManager::CreateAsset<MeshResource>("Monkey.obj");
		Texture2D::Sptr    boxTexture = ResourceManager::CreateAsset<Texture2D>("textures/box-diffuse.png");
		Texture2D::Sptr    monkeyTex  = ResourceManager::CreateAsset<Texture2D>("textures/monkey-uvMap.png");
		
		//// Table
		MeshResource::Sptr mesh_table = ResourceManager::CreateAsset<MeshResource>("gObj_table/table.obj");
		MeshResource::Sptr mesh_table_plane = ResourceManager::CreateAsset<MeshResource>("gObj_table/table_plane.obj");
		//Texture2D::Sptr tex_table = ResourceManager::CreateAsset<Texture2D>("gObj_table/tex_table.png");
		Texture2D::Sptr tex_white = ResourceManager::CreateAsset<Texture2D>("gObj_table/blankTexture.jpg");

		//// Puck
		MeshResource::Sptr mesh_puck = ResourceManager::CreateAsset<MeshResource>("gObj_puck/puck.obj");
		Texture2D::Sptr tex_puck = ResourceManager::CreateAsset<Texture2D>("gObj_puck/Black.jpg");

		//// Paddle
		MeshResource::Sptr mesh_paddle = ResourceManager::CreateAsset<MeshResource>("gObj_paddle/paddle.obj");
		Texture2D::Sptr tex_paddle_red = ResourceManager::CreateAsset<Texture2D>("gObj_paddle/Red.jpg");

		//// Edge
		MeshResource::Sptr mesh_edge1 = ResourceManager::CreateAsset<MeshResource>("gObj_edge/edge_uni.obj");
		MeshResource::Sptr mesh_edge2 = ResourceManager::CreateAsset<MeshResource>("gObj_edge/edge_uni.obj");
		MeshResource::Sptr mesh_edge3 = ResourceManager::CreateAsset<MeshResource>("gObj_edge/edge_uni.obj");
		MeshResource::Sptr mesh_edge4 = ResourceManager::CreateAsset<MeshResource>("gObj_edge/edge_uni.obj");
		MeshResource::Sptr mesh_edge5 = ResourceManager::CreateAsset<MeshResource>("gObj_edge/edge_uni.obj");
		MeshResource::Sptr mesh_edge6 = ResourceManager::CreateAsset<MeshResource>("gObj_edge/edge_uni.obj");
		MeshResource::Sptr mesh_edge7 = ResourceManager::CreateAsset<MeshResource>("gObj_edge/edge_uni.obj");
		MeshResource::Sptr mesh_edge8 = ResourceManager::CreateAsset<MeshResource>("gObj_edge/edge_uni.obj");
		MeshResource::Sptr mesh_edge9 = ResourceManager::CreateAsset<MeshResource>("gObj_edge/edge_uni.obj");
		MeshResource::Sptr mesh_edge10 = ResourceManager::CreateAsset<MeshResource>("gObj_edge/edge_uni.obj");
		MeshResource::Sptr mesh_edge11 = ResourceManager::CreateAsset<MeshResource>("gObj_edge/edge_uni.obj");
		MeshResource::Sptr mesh_edge12 = ResourceManager::CreateAsset<MeshResource>("gObj_edge/edge_uni.obj");
		


		//// ENDREGION
		#pragma endregion	

		// Create an empty scene
		scene = std::make_shared<Scene>();
		// I hate this
		scene->BaseShader = uboShader;

//// Materials ////
		#pragma region Material Creation

		Material::Sptr boxMaterial = ResourceManager::CreateAsset<Material>();
		{
			boxMaterial->Name = "Box";
			boxMaterial->MatShader = scene->BaseShader;
			boxMaterial->Texture = boxTexture;
			boxMaterial->Shininess = 2.0f;
		}

		//// Monkey
		Material::Sptr monkeyMaterial = ResourceManager::CreateAsset<Material>();
		{
			monkeyMaterial->Name = "Monkey";
			monkeyMaterial->MatShader = scene->BaseShader;
			monkeyMaterial->Texture = monkeyTex;
			monkeyMaterial->Shininess = 256.0f;
		}

		//// Table
		Material::Sptr material_table = ResourceManager::CreateAsset<Material>();
		{
			material_table->Name = "Table";
			material_table->MatShader = scene->BaseShader;
			material_table->Texture = tex_white;
			material_table->Shininess = 300.0f;
		}

		//// Puck
		Material::Sptr material_puck = ResourceManager::CreateAsset<Material>();
		{
			material_puck->Name = "Puck";
			material_puck->MatShader = scene->BaseShader;
			material_puck->Texture = tex_puck;
			material_puck->Shininess = 256.0f;
		}

		//// Paddle
		Material::Sptr material_paddle = ResourceManager::CreateAsset<Material>();
		{
			material_paddle->Name = "Paddle";
			material_paddle->MatShader = scene->BaseShader;
			material_paddle->Texture = tex_paddle_red;
			material_paddle->Shininess = 256.0f;
		}

		//// Edge
		Material::Sptr material_edge = ResourceManager::CreateAsset<Material>();
		{
			material_edge->Name = "Edge";
			material_edge->MatShader = scene->BaseShader;
			material_edge->Texture = tex_white;
			material_edge->Shininess = 256.0f;
		}

		//// ENDREGION
		#pragma endregion

//// Lights ////
		#pragma region Lights Creation
		// Create some lights for our scene
		scene->Lights.resize(3);

		// Main Light
		scene->Lights[0].Position = glm::vec3(0.0f, 0.0f, 10.0f);
		scene->Lights[0].Color = glm::vec3(1.0f, 1.0f, 1.0f);
		scene->Lights[0].Range = 300.0f;
		#pragma endregion

//// gObj Setup ////
		#pragma region Scene Object Setup
		//// Camera
		GameObject::Sptr camera = scene->CreateGameObject("Main Camera");
		{
			camera->SetPostion(glm::vec3(0.0f, 0.0f, 10.0f));
			camera->LookAt(glm::vec3(0.0f));
			camera->SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));

			Camera::Sptr cam = camera->Add<Camera>();

			// Make sure that the camera is set as the scene's main camera!
			scene->MainCamera = cam;
		}
		 
		//// Table
		GameObject::Sptr gObj_table = scene->CreateGameObject("Base Table");
		{
			gObj_table->SetPostion(glm::vec3(0.0f, 0.0f, -8.0f));
			
			RenderComponent::Sptr renderer = gObj_table->Add<RenderComponent>();
			renderer->SetMesh(mesh_table);
			renderer->SetMaterial(material_table);

			RigidBody::Sptr physics = gObj_table->Add<RigidBody>(RigidBodyType::Static);
		}
		GameObject::Sptr gObj_table_plane = scene->CreateGameObject("Table_plane");
		{
			gObj_table_plane->SetPostion(glm::vec3(0.0f, 0.0f, -8.01f));

			RenderComponent::Sptr renderer = gObj_table_plane->Add<RenderComponent>();
			renderer->SetMesh(mesh_table_plane);
			renderer->SetMaterial(material_table);

			RigidBody::Sptr physics = gObj_table_plane->Add<RigidBody>(RigidBodyType::Static);
			physics->AddCollider(PlaneCollider::Create());
		}
		
		//// Puck
		GameObject::Sptr gObj_puck = scene->CreateGameObject("Puck");
		{
			gObj_puck->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));
			gObj_puck->SetPostion(glm::vec3(0.0f, 0.0f, 4.0f));


			RenderComponent::Sptr renderer = gObj_puck->Add<RenderComponent>();
			renderer->SetMesh(mesh_puck);
			renderer->SetMaterial(material_puck);

			RigidBody::Sptr physics = gObj_puck->Add<RigidBody>(RigidBodyType::Dynamic);
			ICollider::Sptr collider = physics->AddCollider(ConvexMeshCollider::Create());

			/*
			MaterialSwapBehaviour::Sptr triggerInteraction = gObj_puck->Add<MaterialSwapBehaviour>();
			triggerInteraction->EnterMaterial = boxMaterial;
			triggerInteraction->ExitMaterial = monkeyMaterial;
			*/
			/*
			TriggerVolume::Sptr trigger = gObj_puck->Add<TriggerVolume>();
			trigger->AddCollider(collider);*/

			BounceBehaviour::Sptr bounceTrigger = gObj_puck->Add<BounceBehaviour>();
			
		}

		//// Paddle_red
		GameObject::Sptr gObj_paddle_red = scene->CreateGameObject("Paddle_red");
		{
			gObj_paddle_red->SetPostion(glm::vec3(-5.0f, 0.0f, -8.01f));
			gObj_paddle_red->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));
			

			RenderComponent::Sptr renderer = gObj_paddle_red->Add<RenderComponent>();
			renderer->SetMesh(mesh_paddle);
			renderer->SetMaterial(material_paddle);

			RigidBody::Sptr physics = gObj_paddle_red->Add<RigidBody>(RigidBodyType::Kinematic);
			physics->AddCollider(ConvexMeshCollider::Create());
		}

		//// Edge
		#pragma region 12 Edges
		GameObject::Sptr gObj_edge1 = scene->CreateGameObject("Edge");
		{
			gObj_edge1->SetPostion(glm::vec3(-17.270f, 5.540f, -8.02f));
			gObj_edge1->SetRotation(glm::vec3(0.0f, 0.0f, 86.5f));
			gObj_edge1->SetScale(glm::vec3(2.980f, 1.0f, 1.0f));
			RenderComponent::Sptr renderer = gObj_edge1->Add<RenderComponent>();
			renderer->SetMesh(mesh_edge1);
			renderer->SetMaterial(boxMaterial);
			RigidBody::Sptr physics = gObj_edge1->Add<RigidBody>(RigidBodyType::Static);
			ICollider::Sptr collider = physics->AddCollider(ConvexMeshCollider::Create());
			collider->SetScale(glm::vec3(2.980f, 1.0f, 1.0f));
			TriggerVolume::Sptr volume = gObj_edge1->Add<TriggerVolume>();
			volume->AddCollider(collider);

		}
		GameObject::Sptr gObj_edge2 = scene->CreateGameObject("Edge");
		{
			gObj_edge2->SetPostion(glm::vec3(-17.270f, -5.540f, -8.02f));
			gObj_edge2->SetRotation(glm::vec3(0.0f, 0.0f, 93.5f));
			gObj_edge2->SetScale(glm::vec3(2.980f, 1.0f, 1.0f));
			RenderComponent::Sptr renderer = gObj_edge2->Add<RenderComponent>();
			renderer->SetMesh(mesh_edge2);
			renderer->SetMaterial(boxMaterial);
			RigidBody::Sptr physics = gObj_edge2->Add<RigidBody>(RigidBodyType::Static);
			ICollider::Sptr collider = physics->AddCollider(ConvexMeshCollider::Create());
			collider->SetScale(glm::vec3(2.980f, 1.0f, 1.0f));
		}
		GameObject::Sptr gObj_edge3 = scene->CreateGameObject("Edge");
		{
			gObj_edge3->SetPostion(glm::vec3(-12.790f, 11.3f, -8.02f));
			gObj_edge3->SetRotation(glm::vec3(0.0f, 0.0f, 32.9f));
			gObj_edge3->SetScale(glm::vec3(5.080f, 1.0f, 1.0f));
			RenderComponent::Sptr renderer = gObj_edge3->Add<RenderComponent>();
			renderer->SetMesh(mesh_edge3);
			renderer->SetMaterial(boxMaterial);
			RigidBody::Sptr physics = gObj_edge3->Add<RigidBody>(RigidBodyType::Static);
			ICollider::Sptr collider = physics->AddCollider(ConvexMeshCollider::Create());
			collider->SetScale(glm::vec3(5.080f, 1.0f, 1.0f));
		}
		GameObject::Sptr gObj_edge4 = scene->CreateGameObject("Edge");
		{
			gObj_edge4->SetPostion(glm::vec3(-12.790f, -11.3f, -8.02f));
			gObj_edge4->SetRotation(glm::vec3(0.0f, 0.0f, 147.1f));
			gObj_edge4->SetScale(glm::vec3(5.080f, 1.0f, 1.0f));
			RenderComponent::Sptr renderer = gObj_edge4->Add<RenderComponent>();
			renderer->SetMesh(mesh_edge4);
			renderer->SetMaterial(boxMaterial);
			RigidBody::Sptr physics = gObj_edge4->Add<RigidBody>(RigidBodyType::Static);
			ICollider::Sptr collider = physics->AddCollider(ConvexMeshCollider::Create());
			collider->SetScale(glm::vec3(5.080f, 1.0f, 1.0f));
		}
		GameObject::Sptr gObj_edge5 = scene->CreateGameObject("Edge");
		{
			gObj_edge5->SetPostion(glm::vec3(-4.210f, 12.810f, -8.02f));
			gObj_edge5->SetRotation(glm::vec3(0.0f, 0.0f, 163.7f));
			gObj_edge5->SetScale(glm::vec3(4.430f, 1.0f, 1.0f));
			RenderComponent::Sptr renderer = gObj_edge5->Add<RenderComponent>();
			renderer->SetMesh(mesh_edge5);
			renderer->SetMaterial(boxMaterial);
			RigidBody::Sptr physics = gObj_edge5->Add<RigidBody>(RigidBodyType::Static);
			ICollider::Sptr collider = physics->AddCollider(ConvexMeshCollider::Create());
			collider->SetScale(glm::vec3(4.430f, 1.0f, 1.0f));
		}
		GameObject::Sptr gObj_edge6 = scene->CreateGameObject("Edge");
		{
			gObj_edge6->SetPostion(glm::vec3(-4.210f, -12.810f, -8.02f));
			gObj_edge6->SetRotation(glm::vec3(0.0f, 0.0f, 16.3f));
			gObj_edge6->SetScale(glm::vec3(4.430f, 1.0f, 1.0f));
			RenderComponent::Sptr renderer = gObj_edge6->Add<RenderComponent>();
			renderer->SetMesh(mesh_edge6);
			renderer->SetMaterial(boxMaterial);
			RigidBody::Sptr physics = gObj_edge6->Add<RigidBody>(RigidBodyType::Static);
			ICollider::Sptr collider = physics->AddCollider(ConvexMeshCollider::Create());
			collider->SetScale(glm::vec3(4.430f, 1.0f, 1.0f));
		}
		GameObject::Sptr gObj_edge7 = scene->CreateGameObject("Edge");
		{
			gObj_edge7->SetPostion(glm::vec3(4.210f, 12.810f, -8.02f));
			gObj_edge7->SetRotation(glm::vec3(0.0f, 0.0f, 16.3f));
			gObj_edge7->SetScale(glm::vec3(4.430f, 1.0f, 1.0f));
			RenderComponent::Sptr renderer = gObj_edge7->Add<RenderComponent>();
			renderer->SetMesh(mesh_edge7);
			renderer->SetMaterial(boxMaterial);
			RigidBody::Sptr physics = gObj_edge7->Add<RigidBody>(RigidBodyType::Static);
			ICollider::Sptr collider = physics->AddCollider(ConvexMeshCollider::Create());
			collider->SetScale(glm::vec3(4.430f, 1.0f, 1.0f));
		}
		GameObject::Sptr gObj_edge8 = scene->CreateGameObject("Edge");
		{
			gObj_edge8->SetPostion(glm::vec3(4.210f, -12.810f, -8.02f));
			gObj_edge8->SetRotation(glm::vec3(0.0f, 0.0f, 163.7f));
			gObj_edge8->SetScale(glm::vec3(4.430f, 1.0f, 1.0f));
			RenderComponent::Sptr renderer = gObj_edge8->Add<RenderComponent>();
			renderer->SetMesh(mesh_edge8);
			renderer->SetMaterial(boxMaterial);
			RigidBody::Sptr physics = gObj_edge8->Add<RigidBody>(RigidBodyType::Static);
			ICollider::Sptr collider = physics->AddCollider(ConvexMeshCollider::Create());
			collider->SetScale(glm::vec3(4.430f, 1.0f, 1.0f));
		}
		GameObject::Sptr gObj_edge9 = scene->CreateGameObject("Edge");
		{
			gObj_edge9->SetPostion(glm::vec3(12.790f, 11.3f, -8.02f));
			gObj_edge9->SetRotation(glm::vec3(0.0f, 0.0f, 147.1f));
			gObj_edge9->SetScale(glm::vec3(5.080f, 1.0f, 1.0f));
			RenderComponent::Sptr renderer = gObj_edge9->Add<RenderComponent>();
			renderer->SetMesh(mesh_edge9);
			renderer->SetMaterial(boxMaterial);
			RigidBody::Sptr physics = gObj_edge9->Add<RigidBody>(RigidBodyType::Static);
			ICollider::Sptr collider = physics->AddCollider(ConvexMeshCollider::Create());
			collider->SetScale(glm::vec3(5.080f, 1.0f, 1.0f));
		}
		GameObject::Sptr gObj_edge10 = scene->CreateGameObject("Edge");
		{
			gObj_edge10->SetPostion(glm::vec3(12.790f, -11.3f, -8.02f));
			gObj_edge10->SetRotation(glm::vec3(0.0f, 0.0f, 32.9f));
			gObj_edge10->SetScale(glm::vec3(5.080f, 1.0f, 1.0f));
			RenderComponent::Sptr renderer = gObj_edge10->Add<RenderComponent>();
			renderer->SetMesh(mesh_edge10);
			renderer->SetMaterial(boxMaterial);
			RigidBody::Sptr physics = gObj_edge10->Add<RigidBody>(RigidBodyType::Static);
			ICollider::Sptr collider = physics->AddCollider(ConvexMeshCollider::Create());
			collider->SetScale(glm::vec3(5.080f, 1.0f, 1.0f));
		}
		GameObject::Sptr gObj_edge11 = scene->CreateGameObject("Edge");
		{
			gObj_edge11->SetPostion(glm::vec3(17.270f, 5.540f, -8.02f));
			gObj_edge11->SetRotation(glm::vec3(0.0f, 0.0f, 93.5f));
			gObj_edge11->SetScale(glm::vec3(2.980f, 1.0f, 1.0f));
			RenderComponent::Sptr renderer = gObj_edge11->Add<RenderComponent>();
			renderer->SetMesh(mesh_edge11);
			renderer->SetMaterial(boxMaterial);
			RigidBody::Sptr physics = gObj_edge11->Add<RigidBody>(RigidBodyType::Static);
			ICollider::Sptr collider = physics->AddCollider(ConvexMeshCollider::Create());
			collider->SetScale(glm::vec3(2.980f, 1.0f, 1.0f));
		}
		GameObject::Sptr gObj_edge12 = scene->CreateGameObject("Edge");
		{
			gObj_edge12->SetPostion(glm::vec3(17.270f, -5.540f, -8.02f));
			gObj_edge12->SetRotation(glm::vec3(0.0f, 0.0f, 86.5f));
			gObj_edge12->SetScale(glm::vec3(2.980f, 1.0f, 1.0f));
			RenderComponent::Sptr renderer = gObj_edge12->Add<RenderComponent>();
			renderer->SetMesh(mesh_edge12);
			renderer->SetMaterial(boxMaterial);
			RigidBody::Sptr physics = gObj_edge12->Add<RigidBody>(RigidBodyType::Static);
			ICollider::Sptr collider = physics->AddCollider(ConvexMeshCollider::Create());
			collider->SetScale(glm::vec3(2.980f, 1.0f, 1.0f));
		}
		#pragma endregion

		
		//// Plane 

//// ENDREGION
		#pragma endregion
		
		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("manifest.json");
		// Save the scene to a JSON file
		scene->Save("scene.json");
	}

	// Call scene awake to start up all of our components
	scene->Window = window;
	scene->Awake();

	// We'll use this to allow editing the save/load path
	// via ImGui, note the reserve to allocate extra space
	// for input!
	std::string scenePath = "scene.json"; 
	scenePath.reserve(256); 

	bool isRotating = true;
	float rotateSpeed = 90.0f;

	// Our high-precision timer
	double lastFrame = glfwGetTime();


	BulletDebugMode physicsDebugMode = BulletDebugMode::None;
	float playbackSpeed = 1.0f;

	nlohmann::json editorSceneState;

	bool isFirstClick = true;

///// Game loop /////
#pragma region Game Loop
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		ImGuiHelper::StartFrame();

		// Calculate the time since our last frame (dt)
		double thisFrame = glfwGetTime();
		float dt = static_cast<float>(thisFrame - lastFrame);

		// Showcasing how to use the imGui library!
		bool isDebugWindowOpen = ImGui::Begin("Debugging");
		if (isDebugWindowOpen) {
			// Draws a button to control whether or not the game is currently playing
			static char buttonLabel[64];
			sprintf_s(buttonLabel, "%s###playmode", scene->IsPlaying ? "Exit Play Mode" : "Enter Play Mode");
			if (ImGui::Button(buttonLabel)) {
				// Save scene so it can be restored when exiting play mode
				if (!scene->IsPlaying) {
					editorSceneState = scene->ToJson();
				}

				// Toggle state
				scene->IsPlaying = !scene->IsPlaying;

				// If we've gone from playing to not playing, restore the state from before we started playing
				if (!scene->IsPlaying) {
					scene = nullptr;
					// We reload to scene from our cached state
					scene = Scene::FromJson(editorSceneState);
					// Don't forget to reset the scene's window and wake all the objects!
					scene->Window = window;
					scene->Awake();
				}
			}

			// Make a new area for the scene saving/loading
			ImGui::Separator();
			if (DrawSaveLoadImGui(scene, scenePath)) {
				// C++ strings keep internal lengths which can get annoying
				// when we edit it's underlying datastore, so recalcualte size
				scenePath.resize(strlen(scenePath.c_str()));

				// We have loaded a new scene, call awake to set
				// up all our components
				scene->Window = window;
				scene->Awake();
			}
			ImGui::Separator();
			// Draw a dropdown to select our physics debug draw mode
			if (BulletDebugDraw::DrawModeGui("Physics Debug Mode:", physicsDebugMode)) {
				scene->SetPhysicsDebugDrawMode(physicsDebugMode);
			}
			LABEL_LEFT(ImGui::SliderFloat, "Playback Speed:    ", &playbackSpeed, 0.0f, 10.0f);
			ImGui::Separator();
		}

		// Clear the color and depth buffers
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Update our application level uniforms every frame

		// Draw some ImGui stuff for the lights
		if (isDebugWindowOpen) {
			for (int ix = 0; ix < scene->Lights.size(); ix++) {
				char buff[256];
				sprintf_s(buff, "Light %d##%d", ix, ix);
				// DrawLightImGui will return true if the light was deleted
				if (DrawLightImGui(scene, buff, ix)) {
					// Remove light from scene, restore all lighting data
					scene->Lights.erase(scene->Lights.begin() + ix);
					scene->SetupShaderAndLights();
					// Move back one element so we don't skip anything!
					ix--;
				}
			}
			// As long as we don't have max lights, draw a button
			// to add another one
			if (scene->Lights.size() < scene->MAX_LIGHTS) {
				if (ImGui::Button("Add Light")) {
					scene->Lights.push_back(Light());
					scene->SetupShaderAndLights();
				}
			}
			// Split lights from the objects in ImGui
			ImGui::Separator();
		}

		dt *= playbackSpeed;

		// Perform updates for all components
		scene->Update(dt);

		// Grab shorthands to the camera and shader from the scene
		Camera::Sptr camera = scene->MainCamera;

		// Cache the camera's viewprojection
		glm::mat4 viewProj = camera->GetViewProjection();
		//DebugDrawer::Get().SetViewProjection(viewProj);

		// Update our worlds physics!
		scene->DoPhysics(dt);

		// Draw object GUIs
		if (isDebugWindowOpen) {
			scene->DrawAllGameObjectGUIs();
		}
		
		// The current material that is bound for rendering
		Material::Sptr currentMat = nullptr;
		Shader::Sptr shader = nullptr;

		// Render all our objects
		ComponentManager::Each<RenderComponent>([&](const RenderComponent::Sptr& renderable) {

			// If the material has changed, we need to bind the new shader and set up our material and frame data
			// Note: This is a good reason why we should be sorting the render components in ComponentManager
			if (renderable->GetMaterial() != currentMat) {
				currentMat = renderable->GetMaterial();
				shader = currentMat->MatShader;

				shader->Bind();
				shader->SetUniform("u_CamPos", scene->MainCamera->GetGameObject()->GetPosition());
				currentMat->Apply();
			}

			// Grab the game object so we can do some stuff with it
			GameObject* object = renderable->GetGameObject();

			// Set vertex shader parameters
			shader->SetUniformMatrix("u_ModelViewProjection", viewProj * object->GetTransform());
			shader->SetUniformMatrix("u_Model", object->GetTransform());
			shader->SetUniformMatrix("u_NormalMatrix", glm::mat3(glm::transpose(glm::inverse(object->GetTransform()))));

			// Draw the object
			renderable->GetMesh()->Draw();
		});

		RigidBody::Sptr rigid_puck = scene->FindObjectByName("Puck")->Get<RigidBody>();
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) 
		{
			//std::cout << "APPLYING FORCE" << std::endl;
			rigid_puck->ApplyForce(glm::vec3(0.0f, 10.0f, 0.0f));
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
		{
			//std::cout << "APPLYING FORCE" << std::endl;
			rigid_puck->ApplyForce(glm::vec3(0.0f, -10.0f, 0.0f));
		}

		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		{
			//std::cout << "APPLYING FORCE" << std::endl;
			rigid_puck->ApplyForce(glm::vec3(10.0f, 0.0f, 0.0f));
		}

		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		{
			//std::cout << "APPLYING FORCE" << std::endl;
			rigid_puck->ApplyForce(glm::vec3(-10.0f, 0.0f, 0.0f));
		}

		/*
		int windowX;
		int windowY;
		glfwGetWindowSize(window, &windowX, &windowY);
		double doub_winX = (double)windowX / 2;
		double doub_winY = (double)windowY / 2;

		
		

		glm::vec2 CursorPos = glm::vec2(cursorX / doub_winX, cursorY / doub_winY);
		GameObject::Sptr getPaddle = scene->FindObjectByName("Paddle_red");
		glm::vec3 glCursorPos = glm::vec3(-(1.0f - CursorPos.x), 1.0f - CursorPos.y, 0.0f);

		//glCursorPos = glCursorPos * viewProj;
		
		//std::cout << glCursorPos.x << " " << glCursorPos.y << " " << glCursorPos.z << " " << glCursorPos.w << std::endl;
		*/

		GameObject::Sptr paddle_R = scene->FindObjectByName("Paddle_red");
		if (glfwGetMouseButton(window, 0) == GLFW_PRESS)
		{
			if (!isFirstClick) 
			{
				double cursorX;
				double cursorY;
				glfwGetCursorPos(window, &cursorX, &cursorY);

				int windowX;
				int windowY;
				glfwGetWindowSize(window, &windowX, &windowY);
				double doub_winX = (double)windowX / 2;
				double doub_winY = (double)windowY / 2;

				cursorX = cursorX / doub_winX;
				cursorY = cursorY / doub_winY;

				f0_cursorPosX = f0_cursorPosX / doub_winX;
				f0_cursorPosY = f0_cursorPosY / doub_winY;


				glm::vec3 dCursorPos = glm::vec3(((float)(cursorX - f0_cursorPosX)), -((float)(cursorY - f0_cursorPosY)), 0.0f);
				//std::cout << dCursorPos.x << " " << dCursorPos.y << " " << dCursorPos.z << std::endl;
				paddle_R->SetPostion((paddle_R->GetPosition() + dCursorPos * 20.0f));
			}
			else 
			{
				glfwGetCursorPos(window, &f0_cursorPosX, &f0_cursorPosY);
				isFirstClick = false;
			}
		}
		glfwGetCursorPos(window, &f0_cursorPosX, &f0_cursorPosY);

		


		
		//scene->FindObjectByName("Paddle_red")->SetPostion(glm::vec3(-tempX * 0.2, tempY * 0.2, 0.0f));


		// End our ImGui window
		ImGui::End();

		VertexArrayObject::Unbind();

		lastFrame = thisFrame;
		ImGuiHelper::EndFrame();
		glfwSwapBuffers(window);
	}
//// ENDREGION
#pragma endregion

	// Clean up the ImGui library
	ImGuiHelper::Cleanup();

	// Clean up the resource manager
	ResourceManager::Cleanup();

	// Clean up the toolkit logger so we don't leak memory
	Logger::Uninitialize();
	return 0;
}