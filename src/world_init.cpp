#include "world_init.hpp"
#include "tiny_ecs_registry.hpp"
#include "render_system.hpp"

Entity createAria(RenderSystem* renderer, vec2 pos)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::PLAYER);
	registry.meshPtrs.emplace(entity, &mesh);

	SpriteSheet& sprite_sheet = renderer->getSpriteSheet(SPRITE_SHEET_DATA_ID::PLAYER);
	registry.spriteSheetPtrs.emplace(entity, &sprite_sheet);

	registry.characterProjectileTypes.emplace(entity);
	registry.players.emplace(entity);
	registry.collidables.emplace(entity);

	Animation& animation = registry.animations.emplace(entity);
	animation.sprite_sheet_ptr = &sprite_sheet;
	animation.setState((int)PLAYER_SPRITE_STATES::EAST);
	animation.is_animating = false; // initially stationary

	// set initial component values
	Position& position = registry.positions.emplace(entity);
	position.position = pos;
	position.scale = vec2(63.f, 100.f);

	Velocity& velocity = registry.velocities.emplace(entity);
	velocity.velocity = { 0.f, 0.f };

	Resources& resources = registry.resources.emplace(entity);
	resources.healthBar = createHealthBar(renderer, entity, entity, PLAYER_BAR_X_OFFSET, PLAYER_HEALTH_BAR_Y_OFFSET);
	resources.manaBar = createManaBar(renderer, entity, entity, PLAYER_BAR_X_OFFSET, PLAYER_MANA_BAR_Y_OFFSET);

	Direction& direction = registry.directions.emplace(entity);
	direction.direction = DIRECTION::E;

	PowerUp& powerUp = registry.powerUps.emplace(entity);
	// TOGGLE THESE TO TEST OR GO GOD MODE - enjoy! :)
	/*powerUp.fasterMovement = true;
	powerUp.increasedDamage[ElementType::WATER] = true;
	powerUp.increasedDamage[ElementType::FIRE] = true;
	powerUp.increasedDamage[ElementType::EARTH] = true;
	powerUp.increasedDamage[ElementType::LIGHTNING] = true;
	powerUp.tripleShot[ElementType::WATER] = true;
	powerUp.tripleShot[ElementType::FIRE] = true;
	powerUp.tripleShot[ElementType::EARTH] = true;
	powerUp.tripleShot[ElementType::LIGHTNING] = true;
	powerUp.bounceOffWalls[ElementType::WATER] = true;
	powerUp.bounceOffWalls[ElementType::FIRE] = true;
	powerUp.bounceOffWalls[ElementType::EARTH] = true;
	powerUp.bounceOffWalls[ElementType::LIGHTNING] = true;*/

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::PLAYER,
			EFFECT_ASSET_ID::ANIMATED,
			GEOMETRY_BUFFER_ID::PLAYER});

	return entity;
}

Entity createFloor(RenderSystem* renderer, vec2 pos, vec2 size)
{
	auto entity = Entity();

	// set initial component values
	Position& position = registry.positions.emplace(entity);
	position.scale = size;
	// pos passed in to createFloor assumes top left corner is (x,y)
	position.position = vec2(pos.x + position.scale.x/2, pos.y + position.scale.y/2);

	Floor& floor = registry.floors.emplace(entity);

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::FLOOR,
			EFFECT_ASSET_ID::REPEAT,
			GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}

Entity createTerrain(RenderSystem* renderer, vec2 pos, vec2 size, DIRECTION dir, float speed, bool moveable)
{
	auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Direction& direction = registry.directions.emplace(entity);
	direction.direction = (DIRECTION)dir;

	// The position passed into createTerrain (x,y) assumes the top left corner
	// and size corresponds to width and height
	Position& position = registry.positions.emplace(entity);
	position.position = vec2(pos.x + size.x/2, pos.y + size.y/2);
	position.prev_position = vec2(pos.x + size.x / 2, pos.y + size.y / 2);
	position.scale = size;

	Terrain& terrain = registry.terrain.emplace(entity);
	if (moveable) {
		terrain.moveable = true;
		Velocity& velocity = registry.velocities.emplace(entity);
		velocity.velocity = { speed , 0.f };
	}

	TEXTURE_ASSET_ID tex = 
		(dir == DIRECTION::N) ? TEXTURE_ASSET_ID::NORTH_TERRAIN : 
		(dir == DIRECTION::S ?  TEXTURE_ASSET_ID::SOUTH_TERRAIN : 
		(dir == DIRECTION::E ?  TEXTURE_ASSET_ID::SIDE_TERRAIN : 
			                    TEXTURE_ASSET_ID::GENERIC_TERRAIN));

	registry.collidables.emplace(entity); // Marking terrain as collidable
	registry.renderRequests.insert(
		entity,
		{ tex,
			EFFECT_ASSET_ID::REPEAT,
			GEOMETRY_BUFFER_ID::SPRITE });
	
	return entity;
}
Entity createObstacle(RenderSystem* renderer, vec2 pos, vec2 size, vec2 vel) {
	auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::GHOST_SHEET);
	registry.meshPtrs.emplace(entity, &mesh);

	SpriteSheet& sprite_sheet = renderer->getSpriteSheet(SPRITE_SHEET_DATA_ID::GHOST_SHEET);
	registry.spriteSheetPtrs.emplace(entity, &sprite_sheet);

	Animation& animation = registry.animations.emplace(entity);
	animation.sprite_sheet_ptr = &sprite_sheet;

	Position& position = registry.positions.emplace(entity);
	position.position = pos;

	float scale_factor = size.y / sprite_sheet.frame_height;
	position.scale = vec2(scale_factor * sprite_sheet.frame_width, scale_factor * sprite_sheet.frame_height);

	Velocity& velocity = registry.velocities.emplace(entity);
	velocity.velocity = vel;

	Obstacle& obstacle = registry.obstacles.emplace(entity);
	registry.collidables.emplace(entity); // Marking obstacle as collidable

	createShadow(renderer, entity, TEXTURE_ASSET_ID::GHOST, GEOMETRY_BUFFER_ID::SPRITE);

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::GHOST_SHEET,
			EFFECT_ASSET_ID::ANIMATED,
			GEOMETRY_BUFFER_ID::GHOST_SHEET });

	return entity;
}

Entity createLostSoul(RenderSystem* renderer, vec2 pos) {
	auto entity = Entity();

	registry.lostSouls.emplace(entity);

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::LOST_SOUL);
	registry.meshPtrs.emplace(entity, &mesh);

	SpriteSheet& sprite_sheet = renderer->getSpriteSheet(SPRITE_SHEET_DATA_ID::LOST_SOUL);
	registry.spriteSheetPtrs.emplace(entity, &sprite_sheet);

	Animation& animation = registry.animations.emplace(entity);
	animation.sprite_sheet_ptr = &sprite_sheet;
	animation.setState((int)LOST_SOUL_STATES::EAST_IDLE);
	animation.is_animating = true;

	Position& position = registry.positions.emplace(entity);
	position.position = pos;

	Velocity& velocity = registry.velocities.emplace(entity);
	velocity.velocity = { 0, 0 };

	position.scale = vec2(100, 100);

	registry.collidables.emplace(entity); // Marking obstacle as collidable

	createShadow(renderer, entity, TEXTURE_ASSET_ID::LOST_SOUL, GEOMETRY_BUFFER_ID::SPRITE);

	//flag to render
	if (registry.cutscenes.size() > 0 && registry.cutscenes.components[0].is_cutscene_6) return entity;

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::LOST_SOUL_SHEET,
			EFFECT_ASSET_ID::ANIMATED,
			GEOMETRY_BUFFER_ID::LOST_SOUL });

	return entity;
}


Entity createEnemy(RenderSystem* renderer, vec2 pos, Enemy enemyAttributes)
{
	auto entity = Entity();

	Position& position = registry.positions.emplace(entity);
	position.position = pos;

	Velocity& velocity = registry.velocities.emplace(entity);
	velocity.velocity.x = 50;

	Resources& resources = registry.resources.emplace(entity);
	resources.healthBar = createHealthBar(renderer, entity, entity, 0.f, ENEMY_HEALTH_BAR_Y_OFFSET);

	Enemy& enemy = registry.enemies.emplace(entity);
	enemy = enemyAttributes;
	
	TEXTURE_ASSET_ID texture_asset;
	TEXTURE_ASSET_ID shadow_texture_asset;
	GEOMETRY_BUFFER_ID geom_buffer;
	SPRITE_SHEET_DATA_ID ss_id;
	float scale_factor = 3.f;
	switch (enemy.type) {
	case ElementType::WATER:
		texture_asset = TEXTURE_ASSET_ID::WATER_ENEMY_SHEET;
		shadow_texture_asset = TEXTURE_ASSET_ID::WATER_ENEMY;
		geom_buffer = GEOMETRY_BUFFER_ID::WATER_ENEMY_SHEET;
		ss_id = SPRITE_SHEET_DATA_ID::WATER_ENEMY_SHEET;
		break;
	case ElementType::FIRE:
		texture_asset = TEXTURE_ASSET_ID::FIRE_ENEMY_SHEET;
		shadow_texture_asset = TEXTURE_ASSET_ID::FIRE_ENEMY;
		geom_buffer = GEOMETRY_BUFFER_ID::FIRE_ENEMY_SHEET;
		ss_id = SPRITE_SHEET_DATA_ID::FIRE_ENEMY_SHEET;
		break;
	case ElementType::EARTH:
		texture_asset = TEXTURE_ASSET_ID::EARTH_ENEMY_SHEET;
		shadow_texture_asset = TEXTURE_ASSET_ID::EARTH_ENEMY;
		geom_buffer = GEOMETRY_BUFFER_ID::EARTH_ENEMY_SHEET;
		ss_id = SPRITE_SHEET_DATA_ID::EARTH_ENEMY_SHEET;
		break;
	case ElementType::LIGHTNING:
		texture_asset = TEXTURE_ASSET_ID::LIGHTNING_ENEMY_SHEET;
		shadow_texture_asset = TEXTURE_ASSET_ID::LIGHTNING_ENEMY;
		geom_buffer = GEOMETRY_BUFFER_ID::LIGHTNING_ENEMY_SHEET;
		ss_id = SPRITE_SHEET_DATA_ID::LIGHTNING_ENEMY_SHEET;
		break;
	default:
		// Should never reach here
		texture_asset = TEXTURE_ASSET_ID::FIRE_ENEMY_SHEET;
		shadow_texture_asset = TEXTURE_ASSET_ID::FIRE_ENEMY;
		geom_buffer = GEOMETRY_BUFFER_ID::FIRE_ENEMY_SHEET;
		ss_id = SPRITE_SHEET_DATA_ID::FIRE_ENEMY_SHEET;
		break;
	}

	Mesh& mesh = renderer->getMesh(geom_buffer);
	registry.meshPtrs.emplace(entity, &mesh);

	SpriteSheet& sprite_sheet = renderer->getSpriteSheet(ss_id);
	registry.spriteSheetPtrs.emplace(entity, &sprite_sheet);

	Animation& animation = registry.animations.emplace(entity);
	animation.sprite_sheet_ptr = &sprite_sheet;
	animation.setState((int)ENEMY_STATES::WEST);
	animation.is_animating = true;

	position.scale = vec2({ scale_factor * sprite_sheet.frame_width, scale_factor * sprite_sheet.frame_height });

	createShadow(renderer, entity, shadow_texture_asset, GEOMETRY_BUFFER_ID::SPRITE);

	registry.collidables.emplace(entity);
	registry.renderRequests.insert(
		entity,
		{texture_asset,
		 EFFECT_ASSET_ID::ANIMATED,
		 geom_buffer });

	return entity;
}

Entity createBoss(RenderSystem* renderer, vec2 pos, Enemy enemyAttributes)
{
	auto entity = Entity();

	Boss& boss = registry.bosses.emplace(entity);

	Position& position = registry.positions.emplace(entity);
	position.position = pos;

	position.scale = vec2({ 230, 200 });

	Velocity& velocity = registry.velocities.emplace(entity);
	velocity.velocity.x = 25;

	Resources& resources = registry.resources.emplace(entity);
	resources.maxHealth = 1500.f;
	resources.currentHealth = 1500.f;

	if (registry.players.entities.size() > 0) {
		Entity player = registry.players.entities[0];
		resources.healthBar = createHealthBar(renderer, entity, player, 0.f, BOSS_HEALTH_BAR_Y_OFFSET);
	}
	else {
		resources.healthBar = createHealthBar(renderer, entity, entity, 0.f, -110.f);
	}

	Enemy& enemy = registry.enemies.emplace(entity);
	enemy = enemyAttributes;

	TEXTURE_ASSET_ID textureAsset;
	TEXTURE_ASSET_ID shadowTextureAsset;
	EFFECT_ASSET_ID effectAsset;
	GEOMETRY_BUFFER_ID geomBuffer;
	switch (enemy.type) {
	case ElementType::WATER:
		textureAsset = TEXTURE_ASSET_ID::WATER_BOSS_SHEET;
		shadowTextureAsset = TEXTURE_ASSET_ID::WATER_BOSS;
		effectAsset = EFFECT_ASSET_ID::ANIMATED;
		geomBuffer = GEOMETRY_BUFFER_ID::BOSS;
		break;
	case ElementType::FIRE:
		textureAsset = TEXTURE_ASSET_ID::FIRE_BOSS_SHEET;
		shadowTextureAsset = TEXTURE_ASSET_ID::FIRE_BOSS;
		effectAsset = EFFECT_ASSET_ID::ANIMATED;
		geomBuffer = GEOMETRY_BUFFER_ID::BOSS;
		break;
	case ElementType::EARTH:
		textureAsset = TEXTURE_ASSET_ID::EARTH_BOSS_SHEET;
		shadowTextureAsset = TEXTURE_ASSET_ID::EARTH_BOSS;
		effectAsset = EFFECT_ASSET_ID::ANIMATED;
		geomBuffer = GEOMETRY_BUFFER_ID::BOSS;
		break;
	case ElementType::LIGHTNING:
		textureAsset = TEXTURE_ASSET_ID::LIGHTNING_BOSS_SHEET;
		shadowTextureAsset = TEXTURE_ASSET_ID::LIGHTNING_BOSS;
		effectAsset = EFFECT_ASSET_ID::ANIMATED;
		geomBuffer = GEOMETRY_BUFFER_ID::BOSS;
		break;
	case ElementType::COMBO:
		textureAsset = TEXTURE_ASSET_ID::FINAL_BOSS;
		shadowTextureAsset = TEXTURE_ASSET_ID::FINAL_BOSS_SHADOW;
		effectAsset = EFFECT_ASSET_ID::ANIMATED;
		geomBuffer = GEOMETRY_BUFFER_ID::FINAL_BOSS;
		break;
	default:
		// should never reach here
		textureAsset = TEXTURE_ASSET_ID::WATER_BOSS_SHEET;
		shadowTextureAsset = TEXTURE_ASSET_ID::WATER_BOSS;
		effectAsset = EFFECT_ASSET_ID::ANIMATED;
		geomBuffer = GEOMETRY_BUFFER_ID::BOSS;
		break;
	}

	Mesh& mesh = renderer->getMesh(geomBuffer);
	registry.meshPtrs.emplace(entity, &mesh);

	if (enemy.type == ElementType::COMBO) {
		SpriteSheet& sprite_sheet = renderer->getSpriteSheet(SPRITE_SHEET_DATA_ID::FINAL_BOSS);
		registry.spriteSheetPtrs.emplace(entity, &sprite_sheet);

		Animation& animation = registry.animations.emplace(entity);
		animation.sprite_sheet_ptr = &sprite_sheet;
		animation.setState((int)FINAL_BOSS_SPRITE_STATES::WEST);
		animation.is_animating = true;

		position.scale = vec2({ 2.f * sprite_sheet.frame_width, 2.f * sprite_sheet.frame_height });

		boss.aura = createFinalBossAura(renderer, entity, 0.f, -25.f);
	}
	else {
		SpriteSheet& sprite_sheet = renderer->getSpriteSheet(SPRITE_SHEET_DATA_ID::BOSS);
		registry.spriteSheetPtrs.emplace(entity, &sprite_sheet);

		Animation& animation = registry.animations.emplace(entity);
		animation.sprite_sheet_ptr = &sprite_sheet;
		animation.setState((int)BOSS_STATES::STANDING);
		animation.is_animating = true;

		position.scale = vec2({ 3.f * sprite_sheet.frame_width, 3.f * sprite_sheet.frame_height });
	}
	
	createShadow(renderer, entity, shadowTextureAsset, GEOMETRY_BUFFER_ID::SPRITE);

	registry.collidables.emplace(entity);
	registry.renderRequests.insert(
		entity,
		{ textureAsset,
		 effectAsset,
		 geomBuffer });

	return entity;
}

Entity createFinalBossAura(RenderSystem* renderer, Entity& owner_entity, float x_offset, float y_offset)
{
	auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::FINAL_BOSS_AURA);
	registry.meshPtrs.emplace(entity, &mesh);

	SpriteSheet& sprite_sheet = renderer->getSpriteSheet(SPRITE_SHEET_DATA_ID::FINAL_BOSS_AURA);

	Animation& animation = registry.animations.emplace(entity);
	animation.sprite_sheet_ptr = &sprite_sheet;
	animation.setState((int)FINAL_BOSS_AURA_SPRITE_STATES::NONE);
	animation.is_animating = false;
	
	Follower& follower = registry.followers.emplace(entity);
	follower.owner = owner_entity;
	follower.y_offset = y_offset;
	follower.x_offset = x_offset;

	Position& position = registry.positions.emplace(entity);
	position.scale = vec2(2.f * sprite_sheet.frame_width, 2.f * sprite_sheet.frame_height);

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::FINAL_BOSS_AURA,
		 EFFECT_ASSET_ID::ANIMATED,
		 GEOMETRY_BUFFER_ID::FINAL_BOSS_AURA });

	return entity;
}

Entity createHealthBar(RenderSystem* renderer, Entity& resource_entity, Entity& position_entity, float x_offset, float y_offset)
{
	auto entity = Entity();

	HealthBar& healthBar = registry.healthBars.emplace(entity);
	healthBar.owner = resource_entity;

	Follower& follower = registry.followers.emplace(entity);
	follower.owner = position_entity;
	follower.y_offset = y_offset;
	follower.x_offset = x_offset;

	float width;
	float height;
	float scale_factor;
	TEXTURE_ASSET_ID texture_asset;

	if (registry.players.has(resource_entity)) {
		width = PLAYER_BAR_WIDTH;
		height = PLAYER_BAR_HEIGHT;
		scale_factor = 3.f;
		texture_asset = TEXTURE_ASSET_ID::PLAYER_HEALTH_BAR;
	}
	else if (registry.bosses.has(resource_entity)) {
		width = BOSS_BAR_WIDTH;
		height = BOSS_BAR_HEIGHT;
		scale_factor = 3.5f;
		texture_asset = TEXTURE_ASSET_ID::BOSS_HEALTH_BAR;
	}
	else {
		width = ENEMY_BAR_WIDTH;
		height = ENEMY_BAR_HEIGHT;
		scale_factor = 2.f;
		texture_asset = TEXTURE_ASSET_ID::ENEMY_HEALTH_BAR;
	}

	Resources& resources = registry.resources.get(resource_entity);
	resources.barRatio = (width - height) / width;
	resources.logoRatio = height / width;

	Position& position = registry.positions.emplace(entity);
	position.scale = vec2(scale_factor * width, scale_factor * height);

	registry.renderRequests.insert(
		entity,
		{ texture_asset,
			EFFECT_ASSET_ID::RESOURCE_BAR,
			GEOMETRY_BUFFER_ID::RESOURCE_BAR });

	return entity;
}

Entity createManaBar(RenderSystem* renderer, Entity& resource_entity, Entity& position_entity, float x_offset, float y_offset)
{
	auto entity = Entity();

	ManaBar& manaBar = registry.manaBars.emplace(entity);
	manaBar.owner = resource_entity;

	Follower& follower = registry.followers.emplace(entity);
	follower.owner = position_entity;
	follower.y_offset = y_offset;
	follower.x_offset = x_offset;

	float width;
	float height;
	float scale_factor;
	TEXTURE_ASSET_ID texture_asset;

	if (registry.players.has(resource_entity)) {
		width = PLAYER_BAR_WIDTH;
		height = PLAYER_BAR_HEIGHT;
		scale_factor = 3.f;
		texture_asset = TEXTURE_ASSET_ID::PLAYER_MANA_BAR;
	}
	else {
		width = ENEMY_BAR_WIDTH;
		height = ENEMY_BAR_HEIGHT;
		scale_factor = 2.f;
		texture_asset = TEXTURE_ASSET_ID::ENEMY_MANA_BAR;
	}

	Resources& resources = registry.resources.get(resource_entity);
	resources.barRatio = (width - height) / width;
	resources.logoRatio = height / width;

	Position& position = registry.positions.emplace(entity);
	position.scale = vec2(scale_factor * width, scale_factor * height);

	registry.renderRequests.insert(
		entity,
		{ texture_asset,
			EFFECT_ASSET_ID::RESOURCE_BAR,
			GEOMETRY_BUFFER_ID::RESOURCE_BAR });

	return entity;
}

Entity createHealthPack(RenderSystem* renderer, vec2 pos)
{
	auto entity = Entity();

	HealthPack& health_pack = registry.healthPacks.emplace(entity);

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Position& health_pack_position = registry.positions.emplace(entity);
	health_pack_position.position = pos;
	health_pack_position.scale = vec2(75.f, 75.f);

	registry.collidables.emplace(entity);

	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::HEALTH_PACK,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE
		}
	);

	return entity;
}

Entity createShadow(RenderSystem* renderer, Entity& owner_entity, TEXTURE_ASSET_ID texture, GEOMETRY_BUFFER_ID geom)
{
	auto entity = Entity();

	Shadow& shadow = registry.shadows.emplace(entity);
	shadow.owner = owner_entity;
	shadow.active = false;

	Mesh& mesh = renderer->getMesh(geom);
	registry.meshPtrs.emplace(entity, &mesh);

	Position& owner_position = registry.positions.get(owner_entity);
	Position& position = registry.positions.emplace(entity);
	position.position = owner_position.position;
	position.scale = owner_position.scale;
	shadow.original_size = position.scale;

	registry.renderRequests.insert(
		entity,
		{ texture,
			EFFECT_ASSET_ID::SHADOW,
			geom });

	return entity;
}

Entity createProjectileSelectDisplay(RenderSystem* renderer, Entity& owner_entity, float x_offset, float y_offset)
{
	auto entity = Entity();

	SpriteSheet& sprite_sheet = renderer->getSpriteSheet(SPRITE_SHEET_DATA_ID::PROJECTILE_SELECT_DISPLAY);
	registry.spriteSheetPtrs.emplace(entity, &sprite_sheet);

	CharacterProjectileType& characterProjectileType = registry.characterProjectileTypes.get(owner_entity);
	Animation& animation = registry.animations.emplace(entity);
	animation.sprite_sheet_ptr = &sprite_sheet;
	animation.setState((int) characterProjectileType.projectileType);
	animation.is_animating = false;

	Position& position = registry.positions.emplace(entity);
	float scale_factor = 2.f;
	position.scale = vec2(scale_factor * sprite_sheet.frame_width, scale_factor * sprite_sheet.frame_height);

	Follower& follower = registry.followers.emplace(entity);
	follower.owner = owner_entity;
	follower.y_offset = y_offset;
	follower.x_offset = x_offset;


	ProjectileSelectDisplay& display = registry.projectileSelectDisplays.emplace(entity);
	display.fasterMovement = createPowerUpIndicator(renderer, entity, vec2(24.f, 29.f), TEXTURE_ASSET_ID::FASTER_MOVEMENT, 0.f, -160.f);
	for (int i = 0; i < 4; i++) {
		display.increasedDamage[i] = createPowerUpIndicator(renderer, entity, vec2(5.f, 6.f), TEXTURE_ASSET_ID::DAMAGE_ARROW, 16.f, (i * 60.f) - 75.f);
		display.tripleShot[i] = createPowerUpIndicator(renderer, entity, vec2(9.f, 9.f), TEXTURE_ASSET_ID::TRIPLE_SHOT, -45.f, (i * 60.f) - 108.f);
		display.bounceOffWalls[i] = createPowerUpIndicator(renderer, entity, vec2(9.f, 9.f), TEXTURE_ASSET_ID::BOUNCE, -45.f, (i * 60.f) - 83.f);
	}

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::PROJECTILE_SELECT_DISPLAY,
			EFFECT_ASSET_ID::ANIMATED,
			GEOMETRY_BUFFER_ID::PROJECTILE_SELECT_DISPLAY });

	return entity;
}

Entity createPowerUpIndicator(RenderSystem* renderer, Entity& owner_entity, vec2 size, TEXTURE_ASSET_ID texture, float x_offset, float y_offset)
{
	auto entity = Entity();

	registry.powerUpIndicators.emplace(entity);

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Position& position = registry.positions.emplace(entity);
	float scale_factor = 2.f;
	position.scale = vec2(scale_factor * size.x, scale_factor * size.y);

	SecondaryFollower& follower = registry.secondaryFollowers.emplace(entity);
	follower.owner = owner_entity;
	follower.y_offset = y_offset;
	follower.x_offset = x_offset;

	registry.renderRequests.insert(
		entity,
		{ texture,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}

Entity createExitDoor(RenderSystem* renderer, vec2 pos) {
	auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	SpriteSheet& sprite_sheet = renderer->getSpriteSheet(SPRITE_SHEET_DATA_ID::PORTAL);
	registry.spriteSheetPtrs.emplace(entity, &sprite_sheet);
	
	Animation& animation = registry.animations.emplace(entity);
	animation.sprite_sheet_ptr = &sprite_sheet;
	animation.setState((int)PORTAL_STATES::OPEN);
	animation.is_animating = true;

	Position& position = registry.positions.emplace(entity);
	position.scale = vec2(100.f, 120.f);
	position.position = vec2(pos.x + position.scale.x/2, pos.y + position.scale.y/2);

	registry.exitDoors.emplace(entity);
	registry.collidables.emplace(entity);
	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::PORTAL,
			EFFECT_ASSET_ID::ANIMATED,
			GEOMETRY_BUFFER_ID::PORTAL});

	return entity;
}

Entity createPowerUpBlock(RenderSystem* renderer, pair<string, bool*>* powerUp, vec2 pos) {
	auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	SpriteSheet& sprite_sheet = renderer->getSpriteSheet(SPRITE_SHEET_DATA_ID::POWER_UP_BLOCK);
	registry.spriteSheetPtrs.emplace(entity, &sprite_sheet);

	Animation& animation = registry.animations.emplace(entity);
	animation.sprite_sheet_ptr = &sprite_sheet;
	animation.setState((int)POWER_UP_BLOCK_STATES::ACTIVE);
	animation.rainbow_enabled = true;

	Position& position = registry.positions.emplace(entity);
	position.position = pos;
	position.scale = vec2(90.f, 90.f);

	PowerUpBlock& powerUpBlock = registry.powerUpBlocks.emplace(entity);
	powerUpBlock.powerUpText = powerUp->first;
	powerUpBlock.powerUpToggle = powerUp->second;

	registry.collidables.emplace(entity);
	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::POWER_UP_BLOCK,
			EFFECT_ASSET_ID::ANIMATED,
			GEOMETRY_BUFFER_ID::POWER_UP_BLOCK });

	return entity;
}

Entity createTestSalmon(RenderSystem* renderer, vec2 pos)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SALMON);
	registry.meshPtrs.emplace(entity, &mesh);

	// set initial component values
	Position& position = registry.positions.emplace(entity);
	position.position = pos;
	position.scale = mesh.original_size * 150.f;
	position.scale.x *= -1; // point front to the right; with sprites this wont be a thing?
	// TODO: how to we integrate direction into our entities?

	Velocity& velocity = registry.velocities.emplace(entity);
	velocity.velocity = { 0.f, 0.f };

	Resources& resources = registry.resources.emplace(entity);
	resources.healthBar = createHealthBar(renderer, entity, entity, 0.f, PLAYER_HEALTH_BAR_Y_OFFSET);

	Direction& direction = registry.directions.emplace(entity);
	direction.direction = DIRECTION::E;

	registry.players.emplace(entity);
	registry.collidables.emplace(entity);
	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::TEXTURE_COUNT, // TEXTURE_COUNT indicates that no txture is needed
			EFFECT_ASSET_ID::SALMON,
			GEOMETRY_BUFFER_ID::SALMON });

	return entity;
}

Entity createProjectile(RenderSystem* renderer, vec2 pos, vec2 vel, ElementType elementType, bool hostile, Entity& player) {
	auto entity = Entity();

	Projectile& projectile = registry.projectiles.emplace(entity);
	projectile.type = elementType;
	projectile.hostile = hostile;

	TEXTURE_ASSET_ID textureAsset;
	GEOMETRY_BUFFER_ID geometryBuffer;
	SPRITE_SHEET_DATA_ID spriteSheet;
	switch (projectile.type) {
		case ElementType::WATER:
			textureAsset = TEXTURE_ASSET_ID::WATER_PROJECTILE_SHEET;
			geometryBuffer = GEOMETRY_BUFFER_ID::WATER_PROJECTILE;
			spriteSheet = SPRITE_SHEET_DATA_ID::WATER_PROJECTILE;
			break;
		case ElementType::FIRE:
			textureAsset = TEXTURE_ASSET_ID::FIRE_PROJECTILE_SHEET;
			geometryBuffer = GEOMETRY_BUFFER_ID::FIRE_PROJECTILE;
			spriteSheet = SPRITE_SHEET_DATA_ID::FIRE_PROJECTILE;
			break;
		case ElementType::EARTH:
			textureAsset = TEXTURE_ASSET_ID::EARTH_PROJECTILE_SHEET;
			geometryBuffer = GEOMETRY_BUFFER_ID::EARTH_PROJECTILE_SHEET;
			spriteSheet = SPRITE_SHEET_DATA_ID::EARTH_PROJECTILE_SHEET;
			break;
		case ElementType::LIGHTNING:
			textureAsset = TEXTURE_ASSET_ID::LIGHTNING_PROJECTILE_SHEET;
			geometryBuffer = GEOMETRY_BUFFER_ID::LIGHTNING_PROJECTILE_SHEET;
			spriteSheet = SPRITE_SHEET_DATA_ID::LIGHTNING_PROJECTILE_SHEET;
			break;
		default:
			textureAsset = TEXTURE_ASSET_ID::WATER_PROJECTILE_SHEET;
			geometryBuffer = GEOMETRY_BUFFER_ID::WATER_PROJECTILE;
			spriteSheet = SPRITE_SHEET_DATA_ID::WATER_PROJECTILE;
			break;
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	Mesh& mesh = renderer->getMesh(geometryBuffer);
	registry.meshPtrs.emplace(entity, &mesh);

	SpriteSheet& sprite_sheet = renderer->getSpriteSheet(spriteSheet);
	registry.spriteSheetPtrs.emplace(entity, &sprite_sheet);

	Animation& animation = registry.animations.emplace(entity);
	animation.sprite_sheet_ptr = &sprite_sheet;
	animation.setState((int)PROJECTILE_STATES::MOVING);

	Velocity& velocity = registry.velocities.emplace(entity);
	velocity.velocity = vel;

	// Set initial position and velocity for the projectile
	Position& position = registry.positions.emplace(entity);
	position.position = pos;
	position.angle = atan2(vel.y, vel.x);
	position.scale = vec2(sprite_sheet.frame_width, sprite_sheet.frame_height);

	registry.collidables.emplace(entity);
  if (!hostile) {
	  PowerUp& powerUp = registry.powerUps.get(player);
	  if (powerUp.tripleShot[elementType]) projectile.damage *= 0.5f; // triple shot projectiles are decreased damage
	  if (powerUp.increasedDamage[elementType]) projectile.damage *= 1.5; // increase damage by factor of 1.5
	  if (powerUp.bounceOffWalls[elementType]) projectile.bounces = 2; // allow 2 bounces off walls
  }

	registry.renderRequests.insert(
		entity,
		{	textureAsset,
			EFFECT_ASSET_ID::ANIMATED,
			geometryBuffer });
	return entity;
}

Entity createText(std::string in_text, vec2 pos, float scale, vec3 color)
{
	Entity entity = Entity();

	Position& position = registry.positions.emplace(entity);
	position.position = pos;
	position.scale = vec2(scale, scale);

	Text& text = registry.texts.emplace(entity);
	text.text = in_text;
	text.color = color;

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::TEXTURE_COUNT,
			EFFECT_ASSET_ID::TEXT_2D,
			GEOMETRY_BUFFER_ID::TEXT_2D });

	return entity;
}

Entity createLine(vec2 position, vec2 scale)
{
	Entity entity = Entity();

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::TEXTURE_COUNT,
		 EFFECT_ASSET_ID::TEXTURED,
		 GEOMETRY_BUFFER_ID::DEBUG_LINE });

	// Create motion
	/*
	Motion& motion = registry.motions.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.position = position;
	motion.scale = scale;
	*/

	registry.debugComponents.emplace(entity);
	return entity;
}

Entity createLifeOrb(RenderSystem* renderer, vec2 pos, int piece_number) {
	auto entity = Entity();

	LifeOrb& life_orb = registry.lifeOrbs.emplace(entity);

	Position& position = registry.positions.emplace(entity);
	position.position = pos;
	
	Velocity& velocity = registry.velocities.emplace(entity);
	velocity.velocity = { 0.f,0.f };

	registry.collidables.emplace(entity);

	SPRITE_SHEET_DATA_ID ss_id = SPRITE_SHEET_DATA_ID::LIFE_ORB;
	TEXTURE_ASSET_ID asset = TEXTURE_ASSET_ID::LIFE_ORB;
	GEOMETRY_BUFFER_ID geom_buffer = GEOMETRY_BUFFER_ID::LIFE_ORB;
	float scale_factor = 2.f;
	if (piece_number == 1 || piece_number == 2 || piece_number == 3) {
		ss_id = SPRITE_SHEET_DATA_ID::LIFE_ORB_SHARD;
		asset = TEXTURE_ASSET_ID::LIFE_ORB_SHARD;
		geom_buffer = GEOMETRY_BUFFER_ID::LIFE_ORB_SHARD;
		scale_factor = 3.f;
	}

	Mesh& mesh = renderer->getMesh(geom_buffer);
	registry.meshPtrs.emplace(entity, &mesh);

	SpriteSheet& sprite_sheet = renderer->getSpriteSheet(ss_id);
	registry.spriteSheetPtrs.emplace(entity, &sprite_sheet);
	position.scale = vec2(scale_factor * sprite_sheet.frame_width, scale_factor * sprite_sheet.frame_height);

	Animation& animation = registry.animations.emplace(entity);
	animation.sprite_sheet_ptr = &sprite_sheet;

	registry.renderRequests.insert(
		entity,
		{ asset,
			EFFECT_ASSET_ID::ANIMATED,
			geom_buffer });

	return entity;
}