#include "assignment5.hpp"
#include "parametric_shapes.hpp"

#include "CelestialBody.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/node.hpp"
#include "core/ShaderProgramManager.hpp"

#include <imgui.h>
#include <tinyfiledialogs.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/random.hpp>

#include <clocale>
#include <stdexcept>

edaf80::Assignment5::Assignment5(WindowManager& windowManager) :
	mCamera(0.5f * glm::half_pi<float>(),
	        static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	        0.01f, 1000.0f),
	inputHandler(), mWindowManager(windowManager), window(nullptr)
{
	WindowManager::WindowDatum window_datum{ inputHandler, mCamera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};

	window = mWindowManager.CreateGLFWWindow("EDAF80: Assignment 5", window_datum, config::msaa_rate);
	if (window == nullptr) {
		throw std::runtime_error("Failed to get a window: aborting!");
	}

	bonobo::init();
}

edaf80::Assignment5::~Assignment5()
{
	bonobo::deinit();
}

void
edaf80::Assignment5::run()
{
	srand(time(0));

	// Game variables
	float moveSpeed = 0.1f;
	float boostSpeed = 2.0f;

	float drag = 0.03f;
	float acceleration = 0.01f;
	float cameraLerpSpeed = 0.05f;
	float maxSideWaysRotation = glm::quarter_pi<float>()/2;


	float planet_radius = 100.0f;

	float shipRadius = 1.0f;
	float missileRadius = 0.01f;

	float missileAcceleration = 0.004f;

	float skyBoxRadius = 300.0f;

	const int no_asteroids = 10;
	float asteroid_radius = 3.0f;
	float asteroidGrowSpeed = 0.01f;
	float minAsteroidStartSize = 0.01f;

	float asteroidMaxSpeed = 0.2f;
	float asteroidMinSpeed = 0.1f;

	float asteroidMean = 0.2f;
	float asteroidStd = 0.1f;


	float maxAngleForSpawn = glm::quarter_pi<float>();

	glm::vec3 cameraOffset = glm::vec3(0.0f, 3.0f, 12.0f);

	// Don't edit
	glm::vec4 wasd = glm::vec4(0);
	float missile_v = 0;
	int score = 0;
	bool gameOver = false;
	float currentSpeed = moveSpeed;
	float asteroidSpeed[no_asteroids];
	glm::vec3 ship_v = glm::vec3(0);
	float ellapsed_time_s = 0.0f;


	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	

	// Set up the camera
	mCamera.mWorld.SetTranslate(cameraOffset);
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 3.0f; // 3 m/s => 10.8 km/h

	// Create the shader programs
	ShaderProgramManager program_manager;
	GLuint fallback_shader = 0u;
	program_manager.CreateAndRegisterProgram("Fallback",
	                                         { { ShaderType::vertex, "common/fallback.vert" },
	                                           { ShaderType::fragment, "common/fallback.frag" } },
	                                         fallback_shader);
	if (fallback_shader == 0u) {
		LogError("Failed to load fallback shader");
		return;
	}

	//
	// Todo: Insert the creation of other shader programs.
	//       (Check how it was done in assignment 3.)
	//
	GLuint skybox_shader = 0u;
	program_manager.CreateAndRegisterProgram("Skybox",
		{ { ShaderType::vertex, "EDAF80/skybox.vert" },
		  { ShaderType::fragment, "EDAF80/skybox.frag" } },
		skybox_shader);
	if (skybox_shader == 0u) {
		LogError("Failed to load skybox shader");
		return;
	}

	GLuint phong_shader = 0u;
	program_manager.CreateAndRegisterProgram("Phong",
		{ { ShaderType::vertex, "EDAF80/phong.vert" },
		  { ShaderType::fragment, "EDAF80/phong.frag" } },
		phong_shader);
	if (phong_shader == 0u) {
		LogError("Failed to load phong shader");
		return;
	}

	GLuint asteroidflame_shader = 0u;
	program_manager.CreateAndRegisterProgram("Asteroidflame",
		{ { ShaderType::vertex, "EDAF80/asteroidflame.vert" },
		  { ShaderType::fragment, "EDAF80/asteroidflame.frag" } },
		asteroidflame_shader);
	if (asteroidflame_shader == 0u) {
		LogError("Failed to load asteroidflame shader");
		return;
	}

	GLuint basis_shader = 0u;
	program_manager.CreateAndRegisterProgram("Basis",
		{ { ShaderType::vertex, "common/basis.vert" },
		  { ShaderType::fragment, "common/basis.frag" } },
		basis_shader);
	if (basis_shader == 0u) {
		LogError("Failed to load basis shader");
		return;
	}

	GLuint crosshair_shader = 0u;
	program_manager.CreateAndRegisterProgram("Crosshair",
		{ { ShaderType::vertex, "EDAF80/crosshair.vert" },
		  { ShaderType::fragment, "EDAF80/crosshair.frag" } },
		crosshair_shader);
	if (crosshair_shader == 0u) {
		LogError("Failed to load crosshair shader");
		return;
	}


	auto light_position = glm::vec3(-2.0f, planet_radius * 2.0f, skyBoxRadius/2);
	auto const set_uniforms = [&light_position](GLuint program) {
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
	};
	bool use_normal_mapping = true;
	auto camera_position = mCamera.mWorld.GetTranslation();
	auto ambient = glm::vec3(0.2f, 0.2f, 0.2f);
	auto diffuse = glm::vec3(0.7f, 0.2f, 0.4f);
	auto specular = glm::vec3(1.0f, 1.0f, 1.0f);
	auto shininess = 10.0f;
	auto const phong_set_uniforms = [&use_normal_mapping, &light_position, &camera_position, &ambient, &diffuse, &specular, &shininess](GLuint program) {
		glUniform1i(glGetUniformLocation(program, "use_normal_mapping"), use_normal_mapping ? 1 : 0);
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
		glUniform3fv(glGetUniformLocation(program, "ambient"), 1, glm::value_ptr(ambient));
		glUniform3fv(glGetUniformLocation(program, "diffuse"), 1, glm::value_ptr(diffuse));
		glUniform3fv(glGetUniformLocation(program, "specular"), 1, glm::value_ptr(specular));
		glUniform1f(glGetUniformLocation(program, "shininess"), shininess);
	};
	auto const asteroidflame_set_uniforms = [&use_normal_mapping, &light_position, &camera_position, &ambient, &diffuse, &specular, &shininess, &asteroidMinSpeed, &asteroidMaxSpeed, &ellapsed_time_s](GLuint program) {
		glUniform1i(glGetUniformLocation(program, "use_normal_mapping"), use_normal_mapping ? 1 : 0);
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
		glUniform3fv(glGetUniformLocation(program, "ambient"), 1, glm::value_ptr(ambient));
		glUniform3fv(glGetUniformLocation(program, "diffuse"), 1, glm::value_ptr(diffuse));
		glUniform3fv(glGetUniformLocation(program, "specular"), 1, glm::value_ptr(specular));
		glUniform1f(glGetUniformLocation(program, "shininess"), shininess);
		glUniform1f(glGetUniformLocation(program, "minSpeed"), asteroidMinSpeed);
		glUniform1f(glGetUniformLocation(program, "maxSpeed"), asteroidMaxSpeed);
		glUniform1f(glGetUniformLocation(program, "t"), ellapsed_time_s);
	};
	//

	//
	// Todo: Load your geometry
	//

	// Skybox
	auto skybox_shape = parametric_shapes::createSphere(skyBoxRadius, 100u, 100u);
	if (skybox_shape.vao == 0u) {
		LogError("Failed to retrieve the mesh for the skybox");
		return;
	}
	auto my_cube_map_id = bonobo::loadTextureCubeMap(config::resources_path("textures_added/space_skybox_xyz/posx.png"),
		config::resources_path("textures_added/space_skybox_xyz/negx.png"),
		config::resources_path("textures_added/space_skybox_xyz/posy.png"),
		config::resources_path("textures_added/space_skybox_xyz/negy.png"),
		config::resources_path("textures_added/space_skybox_xyz/posz.png"),
		config::resources_path("textures_added/space_skybox_xyz/negz.png"));

	Node skybox;
	skybox.set_geometry(skybox_shape);
	skybox.set_program(&skybox_shader, set_uniforms);
	skybox.add_texture("my_cube_map", my_cube_map_id, GL_TEXTURE_CUBE_MAP);

	// Planet
	auto planet_shape = parametric_shapes::createSphere(planet_radius, 100u, 100u);
	if (planet_shape.vao == 0u) {
		LogError("Failed to retrieve the mesh for the demo sphere");
		return;
	}
	bonobo::material_data demo_material;
	demo_material.ambient = glm::vec3(0.1f, 0.1f, 0.1f);
	demo_material.shininess = 5.0f;


	auto my_texture_id = bonobo::loadTexture2D(config::resources_path("planets/2k_earth_daymap.jpg"));
	auto my_texture_normal_id = bonobo::loadTexture2D(config::resources_path("planets/2k_earth_normal_map.png"));
	auto my_texture_rough_id = bonobo::loadTexture2D(config::resources_path("planets/2k_earth_daymap_rough.jpg"));

	auto my_asteroid_texture_id = bonobo::loadTexture2D(config::resources_path("planets/2k_moon.jpg"));
	auto my_asteroid_texture_normal_id = bonobo::loadTexture2D(config::resources_path("textures_added/moon_normal.jpg"));
	auto my_asteroid_texture_rough_id = bonobo::loadTexture2D(config::resources_path("textures_added/moon_rough.jpg"));

	Node planet;
	planet.set_geometry(planet_shape);
	planet.set_material_constants(demo_material);
	planet.set_program(&phong_shader, phong_set_uniforms);
	planet.add_texture("my_texture", my_texture_id, GL_TEXTURE_2D);
	planet.add_texture("my_texture_normal", my_texture_normal_id, GL_TEXTURE_2D);
	planet.add_texture("my_texture_rough", my_texture_rough_id, GL_TEXTURE_2D);

	planet.get_transform().SetTranslate(glm::vec3(0.0f, 0.0f, skyBoxRadius* glm::cos(glm::asin(planet_radius / skyBoxRadius))));

	// Ship
	auto ship_shape = bonobo::loadObjects(config::resources_path("scenes/StarSparrow04.obj"));
	if (planet_shape.vao == 0u) {
		LogError("Failed to retrieve the mesh for the ship");
		return;
	}
	Node ship;

	auto my_ship_texture_id = bonobo::loadTexture2D(config::resources_path("scenes/Textures/StarSparrow_Red.png"));
	auto my_ship_texture_normal_id = bonobo::loadTexture2D(config::resources_path("scenes/Textures/StarSparrow_Normal.png"));
	auto my_ship_texture_rough_id = bonobo::loadTexture2D(config::resources_path("scenes/Textures/StarSparrow_Roughness.png"));


	// Ship models
	Node ship_model[1];
	for (int i = 0; i < ship_shape.size(); i++) {
		ship_model[i].add_texture("my_texture", my_ship_texture_id, GL_TEXTURE_2D);
		ship_model[i].add_texture("my_texture_normal", my_ship_texture_normal_id, GL_TEXTURE_2D);
		ship_model[i].add_texture("my_texture_rough", my_ship_texture_rough_id, GL_TEXTURE_2D);
		ship_model[i].set_geometry(ship_shape[i]);
		ship_model[i].set_program(&phong_shader, phong_set_uniforms);
		ship.add_child(&ship_model[i]);
		ship_model[i].get_transform().SetScale(0.5f);
		ship_model[i].set_material_constants(demo_material);
	}



	Node cameraNode;
	cameraNode.get_transform().SetTranslate(cameraOffset);


	// Missile
	auto missile_shape = bonobo::loadObjects(config::resources_path("scenes/rocket.3ds"));
	if (missile_shape.empty()) {
		LogError("Failed to retrieve the mesh for the missile");
		return;
	}
	Node missile;

	auto my_missile_texture_id = bonobo::loadTexture2D(config::resources_path("scenes/Textures/Missile.png"));
	Node missile_model[8];
	bool missileReady = true;
	for (int i = 0; i < missile_shape.size(); i++) {
		missile_model[i].set_geometry(missile_shape[i]);
		missile_model[i].add_texture("my_texture", my_missile_texture_id, GL_TEXTURE_2D);
		missile_model[i].set_program(&phong_shader, phong_set_uniforms);
		missile.add_child(&missile_model[i]);
		missile_model[i].get_transform().SetScale(0.2f);
		missile_model[i].set_material_constants(demo_material);
	}

	// Asteroids
	bonobo::mesh_data asteroid_shape[no_asteroids];
	Node asteroid[no_asteroids];
	bonobo::mesh_data asteroid_flame_shape[no_asteroids];
	Node asteroid_flame[no_asteroids];
	for (int i = 0; i < no_asteroids; i++) {
		asteroidSpeed[i] = glm::linearRand(asteroidMinSpeed, asteroidMaxSpeed);

		glm::vec3 pos_vec = glm::ballRand(1.0f);
		while (glm::dot(pos_vec, glm::vec3(0.0f, 0.0f, 1.0f)) > -glm::cos(maxAngleForSpawn)) {
			pos_vec = glm::ballRand(1.0f);
		}
		asteroid_shape[i] = parametric_shapes::createAsteroid(asteroid_radius, 10u, 10u, asteroidSpeed[i], glm::normalize(planet.get_transform().GetTranslation() - asteroid[i].get_transform().GetTranslation()));

		//
		if (asteroid_shape[i].vao == 0u) {
			LogError("Failed to retrieve the mesh for the demo sphere");
			return;
		}
		asteroid[i].set_geometry(asteroid_shape[i]);
		asteroid[i].set_program(&phong_shader, phong_set_uniforms);
		asteroid[i].add_texture("my_texture", my_asteroid_texture_id, GL_TEXTURE_2D);
		asteroid[i].add_texture("my_texture_normal", my_asteroid_texture_normal_id, GL_TEXTURE_2D);
		asteroid[i].add_texture("my_texture_rough", my_asteroid_texture_rough_id, GL_TEXTURE_2D);
		asteroid[i].set_material_constants(demo_material);


		asteroid[i].get_transform().SetTranslate(pos_vec * skyBoxRadius);

		float asteroidStartSize = glm::min(glm::max(glm::gaussRand(minAsteroidStartSize, 1.0f), asteroidMinSpeed), asteroidMaxSpeed);

		asteroid[i].get_transform().SetScale(asteroidStartSize);

		asteroid_flame_shape[i] = parametric_shapes::createAsteroid(asteroid_radius + 1.0f, 50u, 50u, asteroidSpeed[i], glm::normalize(planet.get_transform().GetTranslation() - asteroid[i].get_transform().GetTranslation()));
		if (asteroid_flame_shape[i].vao == 0u) {
			LogError("Failed to retrieve the mesh for the demo sphere");
			return;
		}
		asteroid_flame[i].set_geometry(asteroid_flame_shape[i]);
		asteroid_flame[i].set_program(&asteroidflame_shader, asteroidflame_set_uniforms);

		asteroid_flame[i].get_transform().SetTranslate(pos_vec * skyBoxRadius);
		asteroid_flame[i].get_transform().SetScale(asteroidStartSize * 1.1f);
	}
	// Crosshair
	//bonobo::mesh_data crosshair_horizontal_shape = parametric_shapes::createQuad(5.0f, 1.1f, 0u, 0u);
	//bonobo::mesh_data crosshair_vertical_shape = parametric_shapes::createQuad(1.1f, 5.0f, 0u, 0u);
	//crosshair_horizontal_shape.drawing_mode = GL_LINES;
	//crosshair_vertical_shape.drawing_mode = GL_LINES;
	//glLineWidth(20);
	//Node crosshair_horizontal;
	//Node crosshair_vertical;
	//crosshair_horizontal.set_geometry(crosshair_horizontal_shape);
	//crosshair_horizontal.set_program(&fallback_shader, phong_set_uniforms);
	//crosshair_vertical.set_geometry(crosshair_vertical_shape);
	//crosshair_vertical.set_program(&fallback_shader, phong_set_uniforms);

	bonobo::mesh_data crosshair_shape = parametric_shapes::createSphere(0.3f, 5u, 5u);
	Node crosshair;
	crosshair.set_geometry(crosshair_shape);
	crosshair.set_program(&crosshair_shader, phong_set_uniforms);


	//

	glClearDepthf(1.0f);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glEnable(GL_DEPTH_TEST);


	auto lastTime = std::chrono::high_resolution_clock::now();

	bool show_logs = true;
	bool show_gui = true;
	bool shader_reload_failed = false;
	bool show_basis = false;
	float basis_thickness_scale = 1.0f;
	float basis_length_scale = 1.0f;


	while (!glfwWindowShouldClose(window)) {
		auto const nowTime = std::chrono::high_resolution_clock::now();
		auto const deltaTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(nowTime - lastTime);
		lastTime = nowTime;

		ellapsed_time_s += std::chrono::duration<float>(deltaTimeUs).count();

		auto& io = ImGui::GetIO();
		inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);



		glfwPollEvents();
		inputHandler.Advance();

		if (inputHandler.GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
			shader_reload_failed = !program_manager.ReloadAllPrograms();
			if (shader_reload_failed)
				tinyfd_notifyPopup("Shader Program Reload Error",
				                   "An error occurred while reloading shader programs; see the logs for details.\n"
				                   "Rendering is suspended until the issue is solved. Once fixed, just reload the shaders again.",
				                   "error");
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
			show_logs = !show_logs;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
			show_gui = !show_gui;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F11) & JUST_RELEASED)
			mWindowManager.ToggleFullscreenStatusForWindow(window);


		// Retrieve the actual framebuffer size: for HiDPI monitors,
		// you might end up with a framebuffer larger than what you
		// actually asked for. For example, if you ask for a 1920x1080
		// framebuffer, you might get a 3840x2160 one instead.
		// Also it might change as the user drags the window between
		// monitors with different DPIs, or if the fullscreen status is
		// being toggled.
		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);


		//
		// Todo: If you need to handle inputs, you can do it here
		//

		// Basic movement input
		if (inputHandler.GetKeycodeState(GLFW_KEY_W) & JUST_PRESSED)
			wasd.x = 1.0f;
		if (inputHandler.GetKeycodeState(GLFW_KEY_W) & JUST_RELEASED)
			wasd.x = 0.0f;

		if (inputHandler.GetKeycodeState(GLFW_KEY_A) & JUST_PRESSED)
			wasd.y = 1.0f;
		if (inputHandler.GetKeycodeState(GLFW_KEY_A) & JUST_RELEASED)
			wasd.y = 0.0f;

		if (inputHandler.GetKeycodeState(GLFW_KEY_S) & JUST_PRESSED)
			wasd.z = 1.0f;
		if (inputHandler.GetKeycodeState(GLFW_KEY_S) & JUST_RELEASED)
			wasd.z = 0.0f;

		if (inputHandler.GetKeycodeState(GLFW_KEY_D) & JUST_PRESSED)
			wasd.w = 1.0f;
		if (inputHandler.GetKeycodeState(GLFW_KEY_D) & JUST_RELEASED)
			wasd.w = 0.0f;
		if (inputHandler.GetKeycodeState(GLFW_KEY_LEFT_SHIFT) & JUST_PRESSED)
			currentSpeed = boostSpeed;
		if (inputHandler.GetKeycodeState(GLFW_KEY_LEFT_SHIFT) & JUST_RELEASED)
			currentSpeed = moveSpeed;

		//  && inputHandler.IsMouseCapturedByUI()

		glm::vec3 missileVec;
		if (missileReady && (inputHandler.GetMouseState(GLFW_MOUSE_BUTTON_LEFT) & PRESSED)) {
			glm::vec2 MousePosition = glm::vec2(inputHandler.GetMousePosition().x, inputHandler.GetMousePosition().y);


			missile.get_transform().SetTranslate(ship.get_transform().GetTranslation());
			for (int i = 0; i < missile_shape.size(); i++) {
				missile_model[i].get_transform().SetTranslate(ship.get_transform().GetTranslation());
			}
			missileReady = false;



			// Simple forward only
			missileVec = ship.get_transform().GetFront();

			missile.get_transform().LookTowards(missileVec);
			for (int i = 0; i < missile_shape.size(); i++) {
				missile_model[i].get_transform().LookTowards(missileVec);
			}
			missile_v = moveSpeed;
		}
		//


		if (!gameOver)
		{
			// -------------------- Ship and camera movement --------------------


			// Camera
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

			mCamera.UpdateNew(deltaTimeUs, inputHandler);
			mCamera.mWorld.SetTranslate(ship.get_transform().GetTranslation());
			

			glm::vec3 lookTowards = mCamera.mWorld.GetBack() * cameraLerpSpeed + ship.get_transform().GetFront() * (1.0f - cameraLerpSpeed);
			glm::vec3 lookUp = mCamera.mWorld.GetUp() * cameraLerpSpeed + ship.get_transform().GetUp() * (1.0f - cameraLerpSpeed);
			
			mCamera.mWorld.SetTranslate(ship.get_transform().GetTranslation() + glm::vec3(glm::vec4(cameraOffset, 1.0f) * glm::inverse(mCamera.mWorld.GetRotationMatrix())));
			//mCamera.mWorld.LookAt(ship.get_transform().GetTranslation());

			// -- Ship --
			// forward
			if (wasd.x == 1) {
				ship_v.z += acceleration - drag * ship_v.z;
			}
			else if (wasd.z == 1) {
				ship_v.z -= acceleration*0.5f + drag * ship_v.z;
			}
			else {
				ship_v.z -= drag * ship_v.z;
			}

			// sideways
			float maxSidewaysSpeed = acceleration * 0.3f / drag;

			if (wasd.y == 1) {
				ship_v.x += acceleration*0.3f - drag * ship_v.x;
			}
			else if (wasd.w == 1) {
				ship_v.x -= acceleration*0.3f + drag * ship_v.x;
			}
			else {
				ship_v.x -= drag * ship_v.x;
			}

			ship.get_transform().SetTranslate(ship.get_transform().GetTranslation() + glm::vec3(glm::vec4(ship_v, 1.0f) * glm::inverse(ship.get_transform().GetRotationMatrix())));

			for (int i = 0; i < ship_shape.size(); i++) {
				ship_model[i].get_transform().SetTranslate(ship.get_transform().GetTranslation() + glm::vec3(glm::vec4(ship_v, 1.0f) * glm::inverse(ship.get_transform().GetRotationMatrix())));
				
			}

			

			// Applying the lerp rotation defined above to ship
			ship.get_transform().LookTowards(lookTowards, lookUp);

			// Finding the separate up vector for the models
			float rotAngle = maxSideWaysRotation * ship_v.x / maxSidewaysSpeed;
			glm::vec3 modelUp = cos(rotAngle) * lookUp + sin(rotAngle) * ship.get_transform().GetRight();

			// Applying the lerp rotation defined above to models
			for (int i = 0; i < ship_shape.size(); i++) {
				ship_model[i].get_transform().LookTowards(lookTowards, modelUp);
			}

			// ------------------- Missile movement ------------------
			missile.get_transform().SetTranslate(missile.get_transform().GetTranslation() - missileVec * missile_v);
			for (int i = 0; i < missile_shape.size(); i++) {
				missile_model[i].get_transform().SetTranslate(missile_model[i].get_transform().GetTranslation() - missileVec * missile_v);
				missile_v += missileAcceleration;
			}

			if (glm::l2Norm(missile.get_transform().GetTranslation()) > skyBoxRadius) {
				missileReady = true;
			}

			// -------------------------------------------------------
			// ------------------ Asteroid movement ------------------
			for (int i = 0; i < no_asteroids; i++) {
				if (glm::l2Norm(asteroid[i].get_transform().GetScale()) < 1.0f) {
					asteroid[i].get_transform().SetScale(asteroid[i].get_transform().GetScale() * (1.0f + asteroidSpeed[i] * asteroidGrowSpeed));
					asteroid_flame[i].get_transform().SetScale(asteroid_flame[i].get_transform().GetScale() * (1.0f + asteroidSpeed[i] * asteroidGrowSpeed));
				}
				else {
					glm::vec3 directionVec = glm::normalize(planet.get_transform().GetTranslation() - asteroid[i].get_transform().GetTranslation());
					asteroid[i].get_transform().SetTranslate(asteroid[i].get_transform().GetTranslation() + directionVec * asteroidSpeed[i]);
					asteroid_flame[i].get_transform().SetTranslate(asteroid_flame[i].get_transform().GetTranslation() + directionVec * asteroidSpeed[i]);
				}
			} 
		} else {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
		// -------------------------------------------------------

		// Crosshair
		glm::vec4 objectVec = glm::vec4(ship.get_transform().GetTranslation() + ship.get_transform().GetFront() * 100.0f, 1.0f);
		glm::vec4 newVec = mCamera.GetWorldToViewMatrix() * objectVec;
		newVec.y = 0;

		crosshair.get_transform().SetTranslate(glm::vec4(mCamera.mWorld.GetTranslation(), 0.0f) - mCamera.mWorld.GetRotationMatrix() * newVec);
		//crosshair_vertical.get_transform().SetTranslate(glm::vec4(mCamera.mWorld.GetTranslation(), 0.0f) - mCamera.mWorld.GetRotationMatrix() * newVec);
		//crosshair_vertical.get_transform().LookAt(mCamera.mWorld.GetTranslation());
		//crosshair_vertical.get_transform().Rotate(glm::half_pi<float>(), mCamera.mWorld.GetUp());
		///

		// --------------- Collision checks ----------------------

		// Missile and asteroids
		for (int i = 0; i < no_asteroids; i++) {
			// If asteroid is hit by projectile
			if (glm::l2Norm(asteroid[i].get_transform().GetTranslation() - missile.get_transform().GetTranslation()) < missileRadius + asteroid_radius) {

				// Randomize new spawn
				glm::vec3 pos_vec = glm::ballRand(1.0f);
				while (glm::dot(pos_vec, glm::vec3(0.0f, 0.0f, 1.0f)) > -glm::cos(maxAngleForSpawn)) {
					pos_vec = glm::ballRand(1.0f);
				}
				asteroid[i].get_transform().SetTranslate(pos_vec * skyBoxRadius);
				asteroid_flame[i].get_transform().SetTranslate(pos_vec * skyBoxRadius);

				// Randomize new size (cooldown timer)
				float asteroidRespawnSize = 0.01;
				asteroid[i].get_transform().SetScale(asteroidRespawnSize);
				asteroid_flame[i].get_transform().SetScale(asteroidRespawnSize * 1.1f);
				score++;
			}
		}

		// Asteroid and planet
		for (int i = 0; i < no_asteroids; i++) {
			if (glm::l2Norm(asteroid[i].get_transform().GetTranslation() - planet.get_transform().GetTranslation()) < planet_radius + asteroid_radius) {
				gameOver = true;
				missileReady = true;
			}
		}


		// -------------------------------------------------------
		mWindowManager.NewImGuiFrame();

		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		// Setting camera position
		camera_position = mCamera.mWorld.GetTranslation();
		//

		if (!shader_reload_failed) {
			//
			// Todo: Render all your geometry here.
			//
			changeCullMode(bonobo::cull_mode_t::back_faces);

			planet.render(mCamera.GetWorldToClipMatrix());
			for (int i = 0; i < ship_shape.size(); i++) {
				ship_model[i].render(mCamera.GetWorldToClipMatrix());
			}
			if (!missileReady) {
				for (int i = 0; i < missile_shape.size(); i++) {
					missile_model[i].render(mCamera.GetWorldToClipMatrix());
				}
			}
			
			

			for (int i = 0; i < no_asteroids; i++) {
				asteroid[i].render(mCamera.GetWorldToClipMatrix());
				asteroid_flame[i].render(mCamera.GetWorldToClipMatrix());
			}

			//crosshair_horizontal.render(mCamera.GetWorldToClipMatrix());
			//crosshair_vertical.render(mCamera.GetWorldToClipMatrix());
			crosshair.render(mCamera.GetWorldToClipMatrix());

			changeCullMode(bonobo::cull_mode_t::disabled);
			skybox.render(mCamera.GetWorldToClipMatrix());
			
		}


		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		//
		// Todo: If you want a custom ImGUI window, you can set it up
		//       here
		//

		//bool const opened = ImGui::Begin("Scene Controls", nullptr, ImGuiWindowFlags_None);
		//if (opened) {
		//	ImGui::Checkbox("Show basis", &show_basis);
		//	ImGui::SliderFloat("Basis thickness scale", &basis_thickness_scale, 0.0f, 100.0f);
		//	ImGui::SliderFloat("Basis length scale", &basis_length_scale, 0.0f, 100.0f);
		//}
		//ImGui::End();



		ImGui::Begin("Planet defense", nullptr, ImGuiWindowFlags_None);
		char txt_green[] = "Missile ready";
		char txt_red[] = "Missile not ready";

		// Particular widget styling
		if (missileReady) {
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
			ImGui::InputText("##text1", txt_green, sizeof(txt_green));
		}
		else {
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
			ImGui::InputText("##text2", txt_red, sizeof(txt_red));
		}
		ImGui::PopStyleColor();
		ImGui::Text("Score: %d", score);
		ImGui::End();

		if (gameOver) {
			ImGui::Begin("Game Over");
			ImGui::Text("Your score is: %d", score);
			ImGui::Separator();
			ImGui::Text("Play again?");
			if (ImGui::Button("Restart game")) {
				// Restart setup
				for (int i = 0; i < no_asteroids; i++) {
					// Randomize new asteroid spawns
					glm::vec3 pos_vec = glm::ballRand(1.0f);
					while (glm::dot(pos_vec, glm::vec3(0.0f, 0.0f, 1.0f)) > -glm::cos(maxAngleForSpawn)) {
						pos_vec = glm::ballRand(1.0f);
					}
					asteroid[i].get_transform().SetTranslate(pos_vec * skyBoxRadius);
					asteroid_flame[i].get_transform().SetTranslate(pos_vec * skyBoxRadius);

					// Randomize new asteroid sizes (cooldown timer)
					float asteroidRespawnSize = glm::linearRand(minAsteroidStartSize, 1.0f);
					asteroid[i].get_transform().SetScale(asteroidRespawnSize);
					asteroid_flame[i].get_transform().SetScale(asteroidRespawnSize * 1.1f);
					score++;

					// Reset ship/player transform
					ship.get_transform().ResetTransform();
					for (int i = 0; i < ship_shape.size(); i++) {
						ship_model[i].get_transform().ResetTransform();
						ship_model[i].get_transform().SetScale(0.5f);
					}

					mCamera.mWorld.SetTranslate(cameraOffset);
					score = 0;
					gameOver = false;

				}
			}
			ImGui::End();
		}
		

		


		if (show_basis)
			bonobo::renderBasis(basis_thickness_scale, basis_length_scale, mCamera.GetWorldToClipMatrix());
		if (show_logs)
			Log::View::Render();
		mWindowManager.RenderImGuiFrame(show_gui);

		glfwSwapBuffers(window);
	}
}

int main()
{
	std::setlocale(LC_ALL, "");

	Bonobo framework;

	try {
		edaf80::Assignment5 assignment5(framework.GetWindowManager());
		assignment5.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
}
