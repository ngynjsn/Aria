#include "game_level.hpp"

bool GameLevel::init(uint level) {
	printf("Initializing game for level: %i\n", level);
	if (level < 0) {
		return false;
	}
	std::vector<vec2>& floors = this->floor_pos;
	std::vector<vec4>& terrains = this->terrains_attr;
	std::vector<std::array<float, ENEMY_ATTRIBUTES>>& enemies = this->enemies_attr;

	switch (level) {
	case 0:
		floors.clear();
		for (uint i = 0; i < 6; i++) {
			for (uint j = 0; j < 4; j++) {
				floors.push_back(vec2(225 + i * 250, 175 + j * 250));
			}
		}
		for (uint i = 0; i < 4; i++) {
			floors.push_back(vec2(1600, 175 + i * 250));
		}

		this->player_starting_pos = vec2(200, 700);
		this->exit_door_pos = vec2(1500, 950);

		terrains.clear();

		terrains.push_back(vec4(900, 0, 1800, 100));
		terrains.push_back(vec4(50, 500, 100, 900));
		terrains.push_back(vec4(900, 1000, 1800, 100));
		terrains.push_back(vec4(1750, 500, 100, 900));
		terrains.push_back(vec4(400, 700, 100, 600));
		terrains.push_back(vec4(800, 300, 100, 600));
		terrains.push_back(vec4(1200, 700, 100, 600));

		enemies.clear();

		enemies.push_back({ 800, 700, 100, 100, 0, 0 });
		enemies.push_back({ 1200, 300, 100, 100, 0, 0 });
		break;
	case 1:
		floors.clear();
		for (uint i = 0; i < 6; i++) {
			for (uint j = 0; j < 4; j++) {
				floors.push_back(vec2(225 + i * 250, 175 + j * 250));
			}
		}
		for (uint i = 0; i < 4; i++) {
			floors.push_back(vec2(1600, 175 + i * 250));
		}

		this->player_starting_pos = vec2(200, 700);
		this->exit_door_pos = vec2(1500, 950);

		terrains.clear();

		terrains.push_back(vec4(900, 0, 1800, 100));
		terrains.push_back(vec4(50, 500, 100, 900));
		terrains.push_back(vec4(900, 1000, 1800, 100));
		terrains.push_back(vec4(1750, 500, 100, 900));
		terrains.push_back(vec4(400, 700, 100, 600));
		terrains.push_back(vec4(800, 300, 100, 600));
		terrains.push_back(vec4(1200, 700, 100, 600));

		enemies.clear();

		enemies.push_back({ 800, 700, 100, 100, 0, 0 });
		enemies.push_back({ 1200, 300, 100, 100, 0, 0 });
		break;
	case 2:
		// Temporary level 2
		this->player_starting_pos = vec2(400, 400);
		terrains.push_back(vec4(600, 400, 100, 100));
		terrains.push_back(vec4(600, 50, 1000, 100));
		terrains.push_back(vec4(50, 400, 100, 800));
		terrains.push_back(vec4(600, 750, 1000, 100));
		terrains.push_back(vec4(1150, 400, 100, 800));

		enemies.push_back({ 300, 600, 100, 100, 0, 0 });
		break;
	default:
		printf("no level provided\n");
		break;
	}


	return true;
}