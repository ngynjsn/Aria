#pragma once

// internal
#include "common.hpp"

// stlib
#include <vector>
#include <random>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>

#include "render_system.hpp"
#include "game_level.hpp"

// Container for all our entities and game logic. Individual rendering / update is
// deferred to the relative update() methods
class WorldSystem
{
public:
	WorldSystem();

	// Creates a window
	GLFWwindow* create_window();

	// starts the game
	void init(RenderSystem* renderer, GameLevel level);

	// Releases all associated resources
	~WorldSystem();

	// Steps the game ahead by ms milliseconds
	bool step(float elapsed_ms);

	// Check for collisions
	void handle_collisions();

	// Should the game be over ?
	bool is_over()const;

	void new_game();
	void win_level();
	void display_power_up();
	GameLevel getLevel() { return curr_level; }
private:
	// Input callback functions
	void on_key(int key, int, int action, int mod);
	void on_scroll(double x_offset, double y_offset);
	void on_mouse_button(int button, int action, int mod);
	void on_mouse_move(vec2 pos);

	// restart game
	void restart_game();

	// OpenGL window handle
	GLFWwindow* window;

	// Game state
	RenderSystem* renderer;
	Entity player;
	Entity projectileSelectDisplay;

	GameLevel curr_level;
	uint next_level = NULL;

	// music references
	Mix_Music* background_music; // TODO: change background music for our game
	Mix_Music* main_menu_music;
	Mix_Music* boss_music;
	Mix_Music* boss_intro_music;
	Mix_Music* final_boss_music;
	Mix_Music* final_boss_intro_music;
	Mix_Music* cutscene_background;
	Mix_Chunk* projectile_sound;
	Mix_Chunk* heal_sound;
	Mix_Chunk* last_enemy_death_sound;
	Mix_Chunk* aria_death_sound;
	Mix_Chunk* enemy_death_sound;
	Mix_Chunk* damage_tick_sound;
	Mix_Chunk* obstacle_collision_sound;
	Mix_Chunk* end_level_sound;
	Mix_Chunk* power_up_sound;
	Mix_Chunk* final_boss_death_sound;

	// cutscene voicelines
	Mix_Chunk* cutscene1_voiceline;
	Mix_Chunk* cutscene2_voiceline;
	Mix_Chunk* cutscene3_voiceline;
	Mix_Chunk* cutscene4_voiceline;
	Mix_Chunk* cutscene5_voiceline;
	Mix_Chunk* cutscene6_voiceline;

	// lost soul voicelines (lsvl)
	Mix_Chunk* fire_boss_lsvl;
	Mix_Chunk* earth_boss_lsvl;
	Mix_Chunk* lightning_boss_lsvl;
	Mix_Chunk* water_boss_lsvl;
	Mix_Chunk* final_boss_lsvl;
	Mix_Chunk* aria_death_lsvl;

	// aria voice lines (avl)
	Mix_Chunk* first_shard_avl;
	Mix_Chunk* third_shard_avl;
	Mix_Chunk* deceived_avl;
	Mix_Chunk* final_cutscene_avl;


	// C++ random number generator
	std::default_random_engine rng;
	std::uniform_real_distribution<float> uniform_dist{0.7, 1}; // number between 0.7 ..1
};
