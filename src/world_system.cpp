// Header
#include "world_system.hpp"
#include "world_init.hpp"
#include "utils.hpp"

// stlib
#include <cassert>
#include <sstream>
#include <iostream>

#include "physics_system.hpp"
using namespace std;

// Game configuration
float PLAYER_SPEED = 300.f;
const float PROJECTILE_SPEED = 700.f;

// Create the world
WorldSystem::WorldSystem() {
	// Seeding rng with random device
	rng = std::default_random_engine(std::random_device()());
}

WorldSystem::~WorldSystem() {
	// Destroy music components
	if (background_music != nullptr)
		Mix_FreeMusic(background_music);
	if (boss_music != nullptr)
		Mix_FreeMusic(boss_music);
	if (boss_intro_music != nullptr)
		Mix_FreeMusic(boss_intro_music);
	if (final_boss_music != nullptr)
		Mix_FreeMusic(final_boss_music);
	if (final_boss_intro_music != nullptr)
		Mix_FreeMusic(final_boss_intro_music);
	if (projectile_sound != nullptr)
		Mix_FreeChunk(projectile_sound);
	if (aria_death_sound != nullptr)
		Mix_FreeChunk(aria_death_sound);
	if (enemy_death_sound != nullptr)
		Mix_FreeChunk(enemy_death_sound);
	if (damage_tick_sound != nullptr)
		Mix_FreeChunk(damage_tick_sound);
	if (end_level_sound != nullptr)
		Mix_FreeChunk(end_level_sound);
	if (power_up_sound != nullptr)
		Mix_FreeChunk(power_up_sound);
	Mix_CloseAudio();

	// Destroy all created components
	registry.clear_all_components();

	// remove ImGui resources
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// Close the window
	glfwDestroyWindow(window);
}

// Debugging
namespace {
	void glfw_err_cb(int error, const char* desc) {
		fprintf(stderr, "%d: %s", error, desc);
	}
}

// World initialization
// Note, this has a lot of OpenGL specific things, could be moved to the renderer
GLFWwindow* WorldSystem::create_window() {
	///////////////////////////////////////
	// Initialize GLFW
	glfwSetErrorCallback(glfw_err_cb);
	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW");
		return nullptr;
	}

	//-------------------------------------------------------------------------
	// If you are on Linux or Windows, you can change these 2 numbers to 4 and 3 and
	// enable the glDebugMessageCallback to have OpenGL catch your mistakes for you.
	// GLFW / OGL Initialization
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#if __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	glfwWindowHint(GLFW_RESIZABLE, 0);

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	// To disable fullscreen-mode, change "monitor" to "nullptr" on next line:
	window = glfwCreateWindow(mode->width, mode->height, "Aria", monitor, nullptr);

	if (window == nullptr) {
		fprintf(stderr, "Failed to glfwCreateWindow");
		return nullptr;
	}
	glfwSetWindowSize(window, window_width_px, window_height_px); // set the resolution

	// Setting callbacks to member functions (that's why the redirect is needed)
	// Input is handled using GLFW, for more info see
	// http://www.glfw.org/docs/latest/input_guide.html
	glfwSetWindowUserPointer(window, this);
	auto key_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2, int _3) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_key(_0, _1, _2, _3); };
	auto mouse_button_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_mouse_button(_0, _1, _2); };
	auto cursor_pos_redirect = [](GLFWwindow* wnd, double _0, double _1) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_mouse_move({ _0, _1 }); };
	glfwSetKeyCallback(window, key_redirect);
	glfwSetMouseButtonCallback(window, mouse_button_redirect);
	glfwSetCursorPosCallback(window, cursor_pos_redirect);

	//////////////////////////////////////
	// Loading music and sounds with SDL
	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		fprintf(stderr, "Failed to initialize SDL Audio");
		return nullptr;
	}
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1) {
		fprintf(stderr, "Failed to open audio device");
		return nullptr;
	}

	background_music = Mix_LoadMUS(audio_path("fast_pace_background.wav").c_str());
	boss_music = Mix_LoadMUS(audio_path("boss_battle.wav").c_str());
	boss_intro_music = Mix_LoadMUS(audio_path("boss_battle_intro.wav").c_str());
	final_boss_music = Mix_LoadMUS(audio_path("final_boss_battle.wav").c_str());
	final_boss_intro_music = Mix_LoadMUS(audio_path("final_boss_battle_intro.wav").c_str());
	projectile_sound = Mix_LoadWAV(audio_path("projectile.wav").c_str());
	aria_death_sound = Mix_LoadWAV(audio_path("aria_death.wav").c_str());
	enemy_death_sound = Mix_LoadWAV(audio_path("enemy_death.wav").c_str());
	damage_tick_sound = Mix_LoadWAV(audio_path("damage_tick.wav").c_str());
	obstacle_collision_sound = Mix_LoadWAV(audio_path("obstacle_collision.wav").c_str());
	end_level_sound = Mix_LoadWAV(audio_path("portal.wav").c_str());
	power_up_sound = Mix_LoadWAV(audio_path("power_up.wav").c_str());

	if (background_music == nullptr) {
		fprintf(stderr, "Failed to load sounds\n %s\n %s\n %s\n make sure the data directory is present",
			audio_path("fast_pace_background.wav").c_str());
		return nullptr;
	}

	return window;
}

void WorldSystem::init(RenderSystem* renderer_arg, GameLevel level) {
	this->renderer = renderer_arg;
	this->curr_level = level;
	// Playing background music indefinitely
	Mix_PlayMusic(background_music, -1);
	fprintf(stderr, "Loaded music\n");

	// Set all states to default
	restart_game();
}

// Update our game world
bool WorldSystem::step(float elapsed_ms_since_last_update) {
	std::stringstream title_ss;
	title_ss << "Aria: Whispers of Darkness";
	glfwSetWindowTitle(window, title_ss.str().c_str());

	// Remove debug info from the last step
	while (registry.debugComponents.entities.size() > 0)
		registry.remove_all_components_of(registry.debugComponents.entities.back());


	assert(registry.screenStates.components.size() <= 1);
	ScreenState& screen = registry.screenStates.components[0];

	for (Entity entity : registry.invulnerableTimers.entities) {
		InvulnerableTimer& timer = registry.invulnerableTimers.get(entity);
		timer.timer_ms -= elapsed_ms_since_last_update;
		if (timer.timer_ms <= 0) {
			registry.invulnerableTimers.remove(entity);
		}
	}

	Resources& player_resource = registry.resources.get(player);
	if (player_resource.currentMana < 10.f) {
		// replenish mana
		player_resource.currentMana += elapsed_ms_since_last_update / 1000;
		if (player_resource.currentMana > 10.f) player_resource.currentMana = 10.f;
	}

    float min_death_timer_ms = 3000.f;
	for (Entity entity : registry.deathTimers.entities) {
		DeathTimer& timer = registry.deathTimers.get(entity);
		timer.timer_ms -= elapsed_ms_since_last_update;
		if (timer.timer_ms < min_death_timer_ms) {
			min_death_timer_ms = timer.timer_ms;
		}
		// restart the game once the death timer expired
		if (timer.timer_ms < 0) {
			registry.deathTimers.remove(entity);
			screen.screen_darken_factor = 0;
			restart_game();
			return true;
		}
	}
	screen.screen_darken_factor = 1 - min_death_timer_ms / 3000;

	float min_win_timer_ms = 3600.f;
	for (Entity entity : registry.winTimers.entities) {
		WinTimer& timer = registry.winTimers.get(entity);
		timer.timer_ms = std::min(timer.timer_ms, min_win_timer_ms);
		timer.timer_ms -= elapsed_ms_since_last_update;
		if (timer.timer_ms > 0.f) {
			screen.apply_spotlight = true;
			screen.spotlight_radius = timer.timer_ms / min_win_timer_ms;
		}
		if (timer.timer_ms <= 0.f) {
			screen.apply_spotlight = true;
			screen.spotlight_radius = -timer.timer_ms / 400.f;

			// Change level here
			if (!timer.changedLevel) {
				timer.changedLevel = true;
				if (this->curr_level.getCurrLevel() != POWER_UP) {
					this->next_level = this->curr_level.getCurrLevel() + 1;
					this->curr_level.init(POWER_UP);
				}
				else {
					this->curr_level.init(this->next_level);
				}
				restart_game();
			}
		}
		if (timer.timer_ms <= -4000.f) {
			registry.winTimers.remove(entity);
			screen.apply_spotlight = false;
		}
	}

	// create exit door once all enemies are dead
	if (registry.enemies.entities.size() == 0 && 
		registry.exitDoors.entities.size() == 0 && 
		this->curr_level.getExitDoorPos() != NULL_POS)
		createExitDoor(renderer, this->curr_level.getExitDoorPos());

	return true;
}

// Reset the world state to its initial state
void WorldSystem::restart_game() {
	// Debugging for memory/component leaks
	registry.list_all_components();
	printf("Restarting\n");

	// hacky solution to persist player components after restart
	bool persistPowerUps = registry.powerUps.has(player);
	PowerUp persistedPowerUps;
	if (persistPowerUps) persistedPowerUps = registry.powerUps.get(player);

	bool persistProjectileType = registry.characterProjectileTypes.has(player);
	CharacterProjectileType persistedProjectileType;
	if (persistProjectileType) persistedProjectileType = registry.characterProjectileTypes.get(player);

	// !!!
	// Remove all entities that we created
	// This might be overkill. Everything that has velocity should already have a position, etc.
	// Just being safe
	while (registry.positions.entities.size() > 0)
	    registry.remove_all_components_of(registry.positions.entities.back());
	while (registry.velocities.entities.size() > 0)
		registry.remove_all_components_of(registry.velocities.entities.back());
	while (registry.resources.entities.size() > 0)
		registry.remove_all_components_of(registry.resources.entities.back());
	while (registry.collidables.entities.size() > 0)
		registry.remove_all_components_of(registry.collidables.entities.back());

	GameLevel current_level = this->curr_level;
	vec2 player_starting_pos = current_level.getPlayerStartingPos();
	uint curr_level = current_level.getCurrLevel();
	std::vector<vec2> floor_pos = current_level.getFloorPos();
	std::vector<std::pair<vec4, bool>> terrains_attrs = current_level.getTerrains();
	std::vector<std::string> texts = current_level.getTexts();
	std::vector<std::array<float, TEXT_ATTRIBUTES>> text_attrs = current_level.getTextAttrs();
	std::vector<std::pair<vec2, Enemy>> enemies_attrs = current_level.getEnemies();
	std::vector<std::pair<vec2, Enemy>> bosses_attrs = current_level.getBosses();
	std::vector<std::array<vec2, OBSTACLE_ATTRIBUTES >> obstacles = current_level.getObstacleAttrs();

	if (curr_level == Level::FIRE_BOSS ||
		curr_level == Level::EARTH_BOSS ||
		curr_level == Level::LIGHTNING_BOSS ||
		curr_level == Level::WATER_BOSS) {
		Mix_FadeInMusic(boss_intro_music, 1, 500);
	}
	else if (curr_level == Level::FINAL_BOSS) {
		Mix_FadeInMusic(final_boss_intro_music, 1, 500);
	}

	// Screen is currently 1200 x 800 (refer to common.hpp to change screen size)
	for (uint i = 0; i < floor_pos.size(); i++) {
		createFloor(renderer, floor_pos[i]);
	}

	player = createAria(renderer, player_starting_pos);

	// ADD BACK THE PERSISTED COMPONENTS
	if (persistPowerUps) registry.powerUps.get(player) = persistedPowerUps;
	if (persistProjectileType) registry.characterProjectileTypes.get(player) = persistedProjectileType;

	for (uint i = 0; i < terrains_attrs.size(); i++) {
		vec4 terrain_i = terrains_attrs[i].first;
		bool moveable = terrains_attrs[i].second;
		createTerrain(renderer, vec2(terrain_i[0], terrain_i[1]), vec2(terrain_i[2], terrain_i[3]), moveable);
	}

	for (uint i = 0; i < texts.size(); i++) {
		std::array<float, TEXT_ATTRIBUTES> text_i = text_attrs[i];
		createText(texts[i], vec2(text_i[0], text_i[1]), text_i[2], vec3(text_i[3], text_i[4], text_i[5]));
	}

	for (uint i = 0; i < enemies_attrs.size(); i++) {
		vec2 pos = enemies_attrs[i].first;
		Enemy enemy = enemies_attrs[i].second;
		createEnemy(renderer, pos, enemy);
	}

	for (uint i = 0; i < bosses_attrs.size(); i++) {
		vec2 pos = bosses_attrs[i].first;
		Enemy enemy = bosses_attrs[i].second;
		createBoss(renderer, pos, enemy);
	}

	for (uint i = 0; i < obstacles.size(); i++) {
		std::array<vec2, OBSTACLE_ATTRIBUTES> obstacle_i = obstacles[i];
		vec2 pos = obstacle_i[0];
		vec2 scale = obstacle_i[1];
		vec2 vel = obstacle_i[2];
		createObstacle(renderer, pos, scale, vel);
	}

	projectileSelectDisplay = createProjectileSelectDisplay(renderer, player, PROJECTILE_SELECT_DISPLAY_Y_OFFSET);

	if (this->curr_level.getCurrLevel() == POWER_UP) display_power_up();

	// Debugging for memory/component leaks
	registry.list_all_components();
}

bool collidedLeft(Position& pos_i, Position& pos_j) 
{
	return (((pos_i.prev_position.x + abs(pos_i.scale.x / 2)) <= (pos_j.position.x - abs(pos_j.scale.x / 2))) &&
		((pos_i.position.x + abs(pos_i.scale.x / 2)) >= (pos_j.position.x - abs(pos_j.scale.x/2))));
}

bool collidedRight(Position& pos_i, Position& pos_j) 
{
	return (((pos_i.prev_position.x - abs(pos_i.scale.x / 2)) >= (pos_j.position.x + abs(pos_j.scale.x / 2))) &&
		((pos_i.position.x - abs(pos_i.scale.x / 2)) <= (pos_j.position.x + abs(pos_j.scale.x/2))));
}

bool collidedTop(Position& pos_i, Position& pos_j) 
{
	return (((pos_i.prev_position.y + abs(pos_i.scale.y / 2)) <= (pos_j.position.y - abs(pos_j.scale.y / 2))) &&
		((pos_i.position.y + abs(pos_i.scale.y / 2)) >= (pos_j.position.y - abs(pos_j.scale.y/2))));
}

bool collidedBottom(Position& pos_i, Position& pos_j) 
{
	return (((pos_i.prev_position.y - abs(pos_i.scale.y / 2)) >= (pos_j.position.y + abs(pos_j.scale.y / 2))) &&
		((pos_i.position.y - abs(pos_i.scale.x / 2)) <= (pos_j.position.x + abs(pos_j.scale.x/2))));
}

void WorldSystem::win_level() {
	if (registry.winTimers.has(player)) return;

	printf("hooray you won the level\n"); 
	registry.velocities.get(player).velocity = { 0.f,0.f };
	registry.winTimers.emplace(player);
	Mix_PlayChannel(-1, end_level_sound, 0);
}

void WorldSystem::display_power_up() {
	PowerUp& powerUp = registry.powerUps.get(player);

	// figure out what power ups are available
	vector<pair<string, bool*>> availPowerUps;

	if (!powerUp.fasterMovement) availPowerUps.push_back(make_pair("Faster Movement Speed", &powerUp.fasterMovement));

	for (int element = ElementType::WATER; element <= ElementType::LIGHTNING; element++) {
		string elementName;
		if (element == ElementType::WATER) elementName = "Water";
		if (element == ElementType::FIRE) elementName = "Fire";
		if (element == ElementType::EARTH) elementName = "Earth";
		if (element == ElementType::LIGHTNING) elementName = "Lightning";

		if (!powerUp.increasedDamage[element]) availPowerUps.push_back(make_pair("Increase " + elementName + " Damage", &powerUp.increasedDamage[element]));
		if (!powerUp.tripleShot[element]) availPowerUps.push_back(make_pair("Triple " + elementName + " Shot", &powerUp.tripleShot[element]));
		if (!powerUp.bounceOffWalls[element]) availPowerUps.push_back(make_pair("Bouncy " + elementName + " Shot", &powerUp.bounceOffWalls[element]));
	}

	if (availPowerUps.size() == 0) {
		printf("No available power ups - DO NOTHING\n");
		return;
	}

	shuffle(availPowerUps.begin(), availPowerUps.end(), rng);
	/*for (int i = 0; i < availPowerUps.size(); i++) {
		printf("%s\n", availPowerUps[i].first.c_str());
	}*/

	if (availPowerUps.size() == 1) {
		createPowerUpBlock(renderer, &availPowerUps[0], vec2(700, 300)); // take top element after shuffling list (randomness!)
	}
	else if (availPowerUps.size() == 2) {
		createPowerUpBlock(renderer, &availPowerUps[0], vec2(575, 300));
		createPowerUpBlock(renderer, &availPowerUps[1], vec2(825, 300));
	}
	else {
		createPowerUpBlock(renderer, &availPowerUps[0], vec2(500, 300));
		createPowerUpBlock(renderer, &availPowerUps[1], vec2(700, 300));
		createPowerUpBlock(renderer, &availPowerUps[2], vec2(900, 300));
	}
}

// Compute collisions between entities
void WorldSystem::handle_collisions() {
	if (registry.deathTimers.has(player) || registry.winTimers.has(player)) { return; } 
	// Loop over all collisions detected by the physics system
	auto& collisionsRegistry = registry.collisions;
	for (uint i = 0; i < collisionsRegistry.components.size(); i++) {
		// The entity and its collider
		Entity entity = collisionsRegistry.entities[i];
		Entity entity_other = collisionsRegistry.components[i].other_entity;

		// Checking Player - Enemy collisions
		if (registry.enemies.has(entity_other) && registry.players.has(entity)) {
			Enemy& enemy = registry.enemies.get(entity_other);
			if (!enemy.isAggravated) {
				enemy.isAggravated = true;

				if (registry.bosses.has(entity_other)) {
					uint curr_level = this->curr_level.getCurrLevel();

					if (curr_level == Level::FIRE_BOSS ||
						curr_level == Level::EARTH_BOSS ||
						curr_level == Level::LIGHTNING_BOSS ||
						curr_level == Level::WATER_BOSS) {
						Mix_FadeInMusic(boss_music, -1, 250);
					}
					else if (curr_level == Level::FINAL_BOSS) {
						Mix_FadeInMusic(final_boss_music, -1, 250);
					}
				}
			}

			if (!registry.invulnerableTimers.has(entity)) {
				Mix_PlayChannel(-1, damage_tick_sound, 0);
				Resources& player_resource = registry.resources.get(entity);
				player_resource.currentHealth -= registry.enemies.get(entity_other).damage;
				printf("player hp: %f\n", player_resource.currentHealth);
				registry.invulnerableTimers.emplace(entity);
				if (player_resource.currentHealth <= 0) {
					registry.deathTimers.emplace(entity);
					registry.velocities.get(player).velocity = { 0.f, 0.f };
					Mix_PlayChannel(-1, aria_death_sound, 0);
				}
			}
		}
		//Checking Player - Obstacle collision
		if (registry.players.has(entity) && registry.obstacles.has(entity_other)) {
			if (!registry.invulnerableTimers.has(entity)) {
				Mix_PlayChannel(-1, obstacle_collision_sound, 0);
				registry.invulnerableTimers.emplace(entity);
				registry.deathTimers.emplace(entity);
				registry.velocities.get(player).velocity = { 0.f, 0.f };
				Mix_PlayChannel(-1, aria_death_sound, 0);
			}
		}

		// Checking Player - Terrain Collisions
		if (registry.players.has(entity) && registry.terrain.has(entity_other)) {
			
			////TODO: do something special when collision with moving wall?
			//if (registry.terrain.get(entity_other).moveable) {
			//}

			Position& player_position = registry.positions.get(entity);
			Position& terrain_position = registry.positions.get(entity_other);
			bool resolved = false;

			if (collidedLeft(player_position, terrain_position) || collidedRight(player_position, terrain_position)) {
				player_position.position.x = player_position.prev_position.x;
				resolved = true;

			}
			if (collidedTop(player_position, terrain_position) || collidedBottom(player_position, terrain_position)) {
				player_position.position.y = player_position.prev_position.y;
				resolved = true;
			}
			if (!resolved) {
				player_position.position += collisionsRegistry.components[i].displacement;
			}
		}
		
		
		// Checking Enemy - Terrain Collisions
		if (registry.enemies.has(entity) && registry.terrain.has(entity_other)) {
			Position& enemy_position = registry.positions.get(entity);
			Position& terrain_position = registry.positions.get(entity_other);
			Velocity& enemy_velocity = registry.velocities.get(entity);
      
			// TODO: make sure enemy has all this stuff and this wont be awful
			// TODO: REFACTOR
			Resources& resources = registry.resources.get(entity);
			HealthBar& health_bar = registry.healthBars.get(resources.healthBar);
			Position& health_bar_position = registry.positions.get(resources.healthBar);
			bool resolved = false;

			if (collidedLeft(enemy_position, terrain_position) || collidedRight(enemy_position, terrain_position)) {
				enemy_position.position.x = enemy_position.prev_position.x;
				enemy_velocity.velocity.x *= -1;
				resolved = true;
			}
			if (collidedTop(enemy_position, terrain_position) || collidedBottom(enemy_position, terrain_position)) {
				enemy_position.position.y = enemy_position.prev_position.y;
				enemy_velocity.velocity.y *= -1;
				resolved = true;
			}
			if (!resolved) {
				enemy_position.position += collisionsRegistry.components[i].displacement;
			}
		}

		// update position of entities that follow player or enemies to remove jitter
		for (int i = 0; i < registry.followers.size(); i++) {
			Follower& follower = registry.followers.components[i];
			Entity entity = registry.followers.entities[i];
			Position& position = registry.positions.get(entity);
			Position& owner_position = registry.positions.get(follower.owner);
			position.position = owner_position.position;
			position.position.y += follower.y_offset;
		}

		// Checking Moveable Terrain - Terrain Collisions
		if (registry.terrain.has(entity) && registry.terrain.has(entity_other)) {
			Terrain& terrain_1 = registry.terrain.get(entity);
			// Checking if the the terrain is moveable
			if (terrain_1.moveable) {
				Velocity& terrain_1_velocity = registry.velocities.get(entity);
				Position& terrain_1_position = registry.positions.get(entity);
				Position& terrain_2_position = registry.positions.get(entity_other);

				if (collidedLeft(terrain_1_position, terrain_2_position) || collidedRight(terrain_1_position, terrain_2_position)) {
					//terrain_1_position.position.x = terrain_1_position.prev_position.x; <- might help
					terrain_1_velocity.velocity[0] = -terrain_1_velocity.velocity[0]; // switch x direction
				}
				if (collidedTop(terrain_1_position, terrain_2_position) || collidedBottom(terrain_1_position, terrain_2_position)) {
					//terrain_1_position.position.y = terrain_1_position.prev_position.y; <- might help
					terrain_1_velocity.velocity[1] = -terrain_1_velocity.velocity[1]; // switch y direction
				}
			}
		}
		//Checking Obstacle Terrain collisions
		if (registry.obstacles.has(entity) && registry.terrain.has(entity_other)) {
				Obstacle& obstacle = registry.obstacles.get(entity);
			
				Velocity& obstacle_velocity = registry.velocities.get(entity);
				Position& obstacle_position = registry.positions.get(entity);
				Position& terrain_position = registry.positions.get(entity_other);

				if (collidedLeft(obstacle_position, terrain_position) || collidedRight(obstacle_position, terrain_position)) {
					obstacle_velocity.velocity[0] = -obstacle_velocity.velocity[0]; // switch x direction
				}
				if (collidedTop(obstacle_position, terrain_position) || collidedBottom(obstacle_position, terrain_position)) {
					obstacle_velocity.velocity[1] = -obstacle_velocity.velocity[1]; // switch y direction
				}
		}
		// Checking Projectile - Enemy collisions
		if (registry.enemies.has(entity_other) && registry.projectiles.has(entity)) {
			if (registry.projectiles.get(entity).hostile && registry.projectiles.get(entity).type != registry.enemies.get(entity_other).type && !registry.bosses.has(entity_other)) {
				// HEAL the target instead
				registry.resources.get(entity_other).currentHealth += 5;
				registry.remove_all_components_of_no_collision(entity); // delete projectile
				if (registry.resources.get(entity_other).currentHealth > registry.resources.get(entity_other).maxHealth) {
					registry.resources.get(entity_other).currentHealth = registry.resources.get(entity_other).maxHealth;
				}
			} else if (!registry.projectiles.get(entity).hostile) {
				Enemy& enemy = registry.enemies.get(entity_other);
				// start boss intro music once aggravated
				if (!enemy.isAggravated && registry.bosses.has(entity_other)) {
					enemy.isAggravated = true;
					uint curr_level = this->curr_level.getCurrLevel();

					if (curr_level == Level::FIRE_BOSS ||
						curr_level == Level::EARTH_BOSS ||
						curr_level == Level::LIGHTNING_BOSS ||
						curr_level == Level::WATER_BOSS) {
						Mix_FadeInMusic(boss_music, -1, 250);
					}
					else if (curr_level == Level::FINAL_BOSS) {
						Mix_FadeInMusic(final_boss_music, -1, 250);
					}
				}
				Mix_PlayChannel(-1, damage_tick_sound, 0);
				Resources& enemy_resource = registry.resources.get(entity_other);
				float damage_dealt = registry.projectiles.get(entity).damage; // any damage modifications should be performed on this value
				if (registry.enemies.get(entity_other).type == registry.projectiles.get(entity).type) {
					enemy_resource.currentHealth = std::min(enemy_resource.maxHealth, enemy_resource.currentHealth + damage_dealt / 2);
				}
				else {
					if (isWeakTo(registry.enemies.get(entity_other).type, registry.projectiles.get(entity).type)) {
						damage_dealt *= 3;
					}
					enemy_resource.currentHealth -= damage_dealt;
				}
		
				registry.remove_all_components_of_no_collision(entity); // delete projectile

				printf("enemy hp: %f\n", enemy_resource.currentHealth);

				// remove enemy if health <= 0
				if (enemy_resource.currentHealth <= 0) {
					bool boss = registry.bosses.has(entity_other); // store bool before removing all components
					registry.remove_all_components_of(enemy_resource.healthBar);
					registry.remove_all_components_of_no_collision(entity_other);
					Mix_PlayChannel(-1, enemy_death_sound, 0);

					// win level and change background music if boss died
					if (boss) {
						win_level();
						Mix_FadeInMusic(background_music, -1, 1500);
					}
				}
			}
		}

		// Checking Projectile - Player collisions
		if (registry.players.has(entity_other) && registry.projectiles.has(entity) && registry.projectiles.get(entity).hostile) {
			Mix_PlayChannel(-1, damage_tick_sound, 0);
			Resources& player_resource = registry.resources.get(entity_other);
			float damage_dealt = registry.projectiles.get(entity).damage; // any damage modifications should be performed on this value
			/* TODO: Can the player be weak to any element?
			if (isWeakTo(registry.players.get(entity_other).type, registry.projectiles.get(entity).type)) {
				damage_dealt *= 2;
			}*/
			player_resource.currentHealth -= damage_dealt;
			printf("Player hp: %f\n", player_resource.currentHealth);
			if (player_resource.currentHealth <= 0) {
				registry.deathTimers.emplace(entity_other);
				registry.velocities.get(player).velocity = vec2(0.f, 0.f);
				Mix_PlayChannel(-1, aria_death_sound, 0);
			}
			registry.remove_all_components_of_no_collision(entity);
		}

		// Checking Terrain - Projectile collisions
		if (registry.terrain.has(entity_other) && registry.projectiles.has(entity)) {
			Projectile& projectile = registry.projectiles.get(entity);

			if (projectile.bounces-- > 0) {
				// bounce the projectile off the wall
				Position& projectile_position = registry.positions.get(entity);
				Velocity& projectile_velocity = registry.velocities.get(entity);
				Position& terrain_position = registry.positions.get(entity_other);

				if (collidedLeft(projectile_position, terrain_position) || collidedRight(projectile_position, terrain_position)) {
					projectile_velocity.velocity.x *= -1;
					projectile_position.angle = atan2(projectile_velocity.velocity.y, projectile_velocity.velocity.x);
				}
				else if (collidedTop(projectile_position, terrain_position) || collidedBottom(projectile_position, terrain_position)) {
					projectile_velocity.velocity.y *= -1;
					projectile_position.angle = atan2(projectile_velocity.velocity.y, projectile_velocity.velocity.x);
				}
			}
			else {
				registry.remove_all_components_of_no_collision(entity);
			}
		}

		// Checking Projectile - Power Up Block collisions
		if (registry.powerUpBlocks.has(entity_other) && registry.projectiles.has(entity)) {
			PowerUpBlock& powerUpBlock = registry.powerUpBlocks.get(entity_other);
			Position& blockPos = registry.positions.get(entity_other);

			// do nothing if this power up is already toggled on
			if (*powerUpBlock.powerUpToggle) {
				registry.remove_all_components_of_no_collision(entity); // remove projectile
				continue;
			}

			// disable previously selected power up first
			auto& powerUpBlocksRegistry = registry.powerUpBlocks;
			for (uint j = 0; j < powerUpBlocksRegistry.entities.size(); j++) {
				Entity pubEntity = powerUpBlocksRegistry.entities[j];
				PowerUpBlock pub = powerUpBlocksRegistry.get(pubEntity);

				if (!*pub.powerUpToggle) continue; // skip over curr power up block if its already disabled

				Animation& animation = registry.animations.get(pubEntity);
				animation.setState((int)POWER_UP_BLOCK_STATES::ACTIVE);
				animation.is_animating = true;
				animation.rainbow_enabled = true;

				*(pub.powerUpToggle) = false;
				registry.remove_all_components_of(pub.textEntity);
			}

			// enable newly selected power up
			Animation& animation = registry.animations.get(entity_other);
			animation.setState((int)POWER_UP_BLOCK_STATES::INACTIVE);
			animation.is_animating = false;
			animation.rainbow_enabled = false;

			*(powerUpBlock.powerUpToggle) = true;
			powerUpBlock.textEntity = createText("You unlocked: " + powerUpBlock.powerUpText, vec2(0.f, 50.f), 1.f, vec3(0.f, 1.f, 0.f));

			Mix_PlayChannel(-1, power_up_sound, 0);

			registry.remove_all_components_of_no_collision(entity); // remove projectile
		}

		// Checking Player - Exit Door collision
		if (registry.players.has(entity) && registry.exitDoors.has(entity_other)) {
			win_level();
		}
	}
	registry.collisions.clear();
}

// Should the game be over ?
bool WorldSystem::is_over() const {
	return bool(glfwWindowShouldClose(window));
}

// On key callback
void WorldSystem::on_key(int key, int, int action, int mod) {
	//Disables keys when death or win timer happening
	if (registry.deathTimers.has(player) || registry.winTimers.has(player)) { return; }

	Velocity& player_velocity = registry.velocities.get(player);
	Position& player_position = registry.positions.get(player);
	Direction& player_direction = registry.directions.get(player);
	Animation& player_animation = registry.animations.get(player);

	// get states of each arrow key
	int state_up = glfwGetKey(window, GLFW_KEY_W);
	int state_down = glfwGetKey(window, GLFW_KEY_S);
	int state_left = glfwGetKey(window, GLFW_KEY_A);
	int state_right = glfwGetKey(window, GLFW_KEY_D);

	DIRECTION prev_direction = player_direction.direction;
	bool was_mirrored = player_position.scale.x < 0;

	DIRECTION new_direction = DIRECTION::NONE;

	// up
	if ((state_up == GLFW_PRESS && state_down == GLFW_RELEASE && state_left == GLFW_RELEASE && state_right == GLFW_RELEASE) ||
		(state_up == GLFW_PRESS && state_down == GLFW_RELEASE && state_left == GLFW_PRESS && state_right == GLFW_PRESS)) {
		new_direction = DIRECTION::N;
	}
	// down
	else if ((state_down == GLFW_PRESS && state_up == GLFW_RELEASE && state_left == GLFW_RELEASE && state_right == GLFW_RELEASE) ||
		(state_down == GLFW_PRESS && state_up == GLFW_RELEASE && state_left == GLFW_PRESS && state_right == GLFW_PRESS)) {
		new_direction = DIRECTION::S;
	}
	// left
	else if ((state_left == GLFW_PRESS && state_down == GLFW_RELEASE && state_up == GLFW_RELEASE && state_right == GLFW_RELEASE) ||
		(state_left == GLFW_PRESS && state_down == GLFW_PRESS && state_up == GLFW_PRESS && state_right == GLFW_RELEASE)) {
		new_direction = DIRECTION::W;
	}
	// right
	else if ((state_right == GLFW_PRESS && state_down == GLFW_RELEASE && state_left == GLFW_RELEASE && state_up == GLFW_RELEASE) ||
		(state_right == GLFW_PRESS && state_down == GLFW_PRESS && state_left == GLFW_RELEASE && state_up == GLFW_PRESS)) {
		new_direction = DIRECTION::E;
	}
	// up and left
	else if ((state_up == GLFW_PRESS && state_down == GLFW_RELEASE && state_left == GLFW_PRESS && state_right == GLFW_RELEASE)) {
		new_direction = DIRECTION::NW;
	}
	// up and right
	else if ((state_up == GLFW_PRESS && state_down == GLFW_RELEASE && state_left == GLFW_RELEASE && state_right == GLFW_PRESS)) {
		new_direction = DIRECTION::NE;
	}
	// down and right
	else if ((state_up == GLFW_RELEASE && state_down == GLFW_PRESS && state_left == GLFW_PRESS && state_right == GLFW_RELEASE)) {
		new_direction = DIRECTION::SW;
	}
	// down and left
	else if ((state_up == GLFW_RELEASE && state_down == GLFW_PRESS && state_left == GLFW_RELEASE && state_right == GLFW_PRESS)) {
		new_direction = DIRECTION::SE;
	}
	
	// Handle key presses for changing projectile type
	if (action == GLFW_PRESS) {
		CharacterProjectileType& characterProjectileType = registry.characterProjectileTypes.get(player);

		switch (key) {
		case GLFW_KEY_1:
			characterProjectileType.projectileType = ElementType::WATER;
			break;
		case GLFW_KEY_2:
			characterProjectileType.projectileType = ElementType::FIRE;
			break;
		case GLFW_KEY_3:
			characterProjectileType.projectileType = ElementType::EARTH;
			break;
		case GLFW_KEY_4:
			characterProjectileType.projectileType = ElementType::LIGHTNING;
			break;
		case GLFW_KEY_9:
			win_level();
			break;
		case GLFW_KEY_0:
			registry.resources.get(player).maxHealth = 10000.f;
			registry.resources.get(player).currentHealth = 10000.f;
			registry.resources.get(player).maxMana = 10000.f;
			registry.resources.get(player).currentMana = 10000.f;
			PLAYER_SPEED = 500;
			break;
		}

		Animation& select_display = registry.animations.get(projectileSelectDisplay);
		select_display.setState((int)characterProjectileType.projectileType);
	}

	// player is moving
	if (new_direction != DIRECTION::NONE) {
		// velocity
		player_direction.direction = new_direction;
		PowerUp& player_powerUp = registry.powerUps.get(player);
		player_velocity = computeVelocity(player_powerUp.fasterMovement ? PLAYER_SPEED * 1.5 : PLAYER_SPEED, player_direction);

		// animation
		if (prev_direction != new_direction) {
			int new_state = player_animation.sprite_sheet_ptr->getPlayerStateFromDirection(new_direction);
			player_animation.setState(new_state);
			bool mirror = player_animation.sprite_sheet_ptr->getPlayerMirrored(new_direction);
			if (was_mirrored != mirror) {
				player_position.scale.x *= -1;
			}
		}
		player_animation.is_animating = true;
	}
	// player is stopped
	else {
		player_velocity = computeVelocity(0.0, player_direction);
		player_animation.is_animating = false;
	}

	// Resetting game
	if (action == GLFW_RELEASE && key == GLFW_KEY_R) {
		int w, h;
		glfwGetWindowSize(window, &w, &h);
		restart_game();
	}

	// Close program
	if (action == GLFW_RELEASE && key == GLFW_KEY_BACKSPACE) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}

	// Debugging
	//if (key == GLFW_KEY_D) {
	//	if (action == GLFW_RELEASE)
	//		debugging.in_debug_mode = false;
	//	else
	//		debugging.in_debug_mode = true;
	//}
}

void WorldSystem::on_mouse_button(int button, int action, int mod) {
	//Disables mouse when death or win timer happening
	if (registry.deathTimers.has(player) || registry.winTimers.has(player)) { return; }

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		// check mana
		if (registry.resources.get(player).currentMana < 1) {
			return;
		} else {
			registry.resources.get(player).currentMana -= 1;
		}

		// get cursor position
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		int width, height;
		glfwGetWindowSize(window, &width, &height);

		// calculate angle
		float deltaX = xpos - (width / 2);
		float deltaY = ypos - (height / 2);
		float angle = atan2(deltaY, deltaX);

		// create projectile
		Position& position = registry.positions.get(player);
		vec2 proj_position = position.position;
		ElementType elementType = registry.characterProjectileTypes.get(player).projectileType; // Get current player projectile type

		// apply power ups
		PowerUp& powerUp = registry.powerUps.get(player);
		
		if (powerUp.tripleShot[elementType]) {
			Velocity vel1 = computeVelocity(PROJECTILE_SPEED, angle - 0.25);
			Velocity vel2 = computeVelocity(PROJECTILE_SPEED, angle);
			Velocity vel3 = computeVelocity(PROJECTILE_SPEED, angle + 0.25);

			Entity projectile1 = createProjectile(renderer, proj_position, vel1.velocity, elementType, false, player);
			Entity projectile2 = createProjectile(renderer, proj_position, vel2.velocity, elementType, false, player);
			Entity projectile3 = createProjectile(renderer, proj_position, vel3.velocity, elementType, false, player);
		}
		else {
			Velocity vel = computeVelocity(PROJECTILE_SPEED, angle);
			Entity projectile = createProjectile(renderer, proj_position, vel.velocity, elementType, false, player);
		}
		Mix_PlayChannel(-1, projectile_sound, 0);
	}
}

void WorldSystem::on_mouse_move(vec2 mouse_position) {
	// We are probably not going to need this on_mouse_move
	
	(vec2)mouse_position; // dummy to avoid compiler warning
}
