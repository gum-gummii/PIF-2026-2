#include <raylib.h>
#include <resource_dir.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <classes.h>
#include <combat.h>
#include <battle.h>
#include <encounter.h>
#include <score.h>
#include <minions.h>
#include <progression.h>
#include <scene_overlay.h>
#include <door.h>
#include <cutscene.h>
#include <time.h>







const bool DEBUG_MODE = true;
const float DMG_COOLDOWN = 0.8;

int playerSpiritSize = 40;
float dmgDebounce = 0;
float clockTimestampMs = 0;
Vector2 worldSize = {0, 0};
typedef enum {
	SCENARIO_GARDEN = 0,
	SCENARIO_ENTRANCE = 1,
	SCENARIO_BOSS_ROOM = 8
} ScenarioId;

typedef struct {
	int scenario;
	Texture2D texture;
} ScenarioBackground;

const float ENTRANCE_ART_HEIGHT = 750.0f;
const float BOSS_ROOM_ART_HEIGHT = 748.0f;
const int START_SCENARIO = SCENARIO_ENTRANCE;

int GetBattleSquareSize() { return GetScreenHeight() - 50; }
int GetBattleBorderSize() { return (int)round(GetBattleSquareSize() / 50); }
Vector2 GetCenterOfScreen() { return (Vector2){GetScreenWidth() / 2, GetScreenHeight() / 2}; }



void DrawBattleBackground(int squareSize) {
	float x = (GetScreenWidth() / 2) - (squareSize / 2);
	float y = (GetScreenHeight() / 2) - (squareSize / 2);

	Rectangle box = {x, y, squareSize, squareSize};
	Color bgColor = {0, 0, 0, 100};

	DrawRectangleRec(box, bgColor);
	DrawRectangleLinesEx(box, GetBattleBorderSize(), GREEN);
}

void MoveSpirit(Spirit* plrSpirit) {
	Vector2 center    = GetCenterOfScreen();
	float speed       = plrSpirit->movespeed;
	float sqrtSpeed   = speed / sqrt(2);

	float leftLimit   = center.x - (GetBattleSquareSize() / 2) + (playerSpiritSize / 2) + GetBattleBorderSize();
	float topLimit    = center.y - (GetBattleSquareSize() / 2) + (playerSpiritSize / 2) + GetBattleBorderSize();
	float rightLimit  = center.x - (playerSpiritSize / 2) + (GetBattleSquareSize() / 2) - GetBattleBorderSize();
	float bottomLimit = center.y - (playerSpiritSize / 2) + (GetBattleSquareSize() / 2) - GetBattleBorderSize();


	bool hasVerticalMovement   = IsKeyDown(KEY_UP)   || IsKeyDown(KEY_W) || IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S);
	bool hasHorizontalMovement = IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A) || IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);

	float verticalStep   = hasHorizontalMovement ? sqrtSpeed : speed;
	float horizontalStep = hasVerticalMovement ? sqrtSpeed : speed;

	if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
		plrSpirit->position.x -= horizontalStep;
	}

	if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
		plrSpirit->position.x += horizontalStep;
	}

	if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
		plrSpirit->position.y -= verticalStep;
	}

	if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
		plrSpirit->position.y += verticalStep;
	}

	plrSpirit->position.x = fminf(fmaxf(plrSpirit->position.x, leftLimit), rightLimit);
	plrSpirit->position.y = fminf(fmaxf(plrSpirit->position.y, topLimit), bottomLimit);
}

Rectangle GetPlayerCollider(Vector2 position, Rectangle plrFrame) {
	return (Rectangle){position.x + plrFrame.width * 0.28f, position.y + plrFrame.height * 0.80f, plrFrame.width * 0.46f, plrFrame.height * 0.17f};
}

float MovePlrAxis(Character* plr, Rectangle plrFrame, float delta, bool horizontal, bool ignoreWalls, LoadedAssetsClass* Assets) {
	float maxX = worldSize.x - plrFrame.width;
	float maxY = worldSize.y - plrFrame.height;
	float step = delta;

	while (fabsf(step) >= 1.0f) {
		Vector2 next = plr->position;
		if (horizontal) { next.x = fminf(fmaxf(next.x + step, 0), maxX); }
		else            { next.y = fminf(fmaxf(next.y + step, 0), maxY); }

		if (ignoreWalls || !Assets->CheckColisions(Assets, GetPlayerCollider(next, plrFrame))) {
			float moved = horizontal ? next.x - plr->position.x : next.y - plr->position.y;
			plr->position = next;
			return moved;
		}
		step += (step > 0) ? -1.0f : 1.0f;
	}
	return 0;
}

bool MovePlr(Character* plr, Rectangle plrFrame, Vector2* plrCurrentFrame, LoadedAssetsClass* Assets) {
	bool up    = IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W);
	bool down  = IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S);
	bool left  = IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A);
	bool right = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
	bool hasVerticalMovement   = up || down;
	bool hasHorizontalMovement = left || right;

	float speed = plr->movespeed;
	float step = (hasVerticalMovement && hasHorizontalMovement) ? speed / sqrtf(2) : speed;
	bool ignoreWalls = Assets->CheckColisions(Assets, GetPlayerCollider(plr->position, plrFrame));
	bool moved = false;

	if (down  && MovePlrAxis(plr, plrFrame,  step, false, ignoreWalls, Assets) != 0) { plrCurrentFrame->y = 1; moved = true; }
	if (up    && MovePlrAxis(plr, plrFrame, -step, false, ignoreWalls, Assets) != 0) { plrCurrentFrame->y = 3; moved = true; }
	if (left  && MovePlrAxis(plr, plrFrame, -step, true,  ignoreWalls, Assets) != 0) { plrCurrentFrame->y = 2; moved = true; }
	if (right && MovePlrAxis(plr, plrFrame,  step, true,  ignoreWalls, Assets) != 0) { plrCurrentFrame->y = 4; moved = true; }

	if (dmgDebounce - clockTimestampMs <= 0 && moved) {
		float dmg = Assets->CheckDamageColisions(Assets, GetPlayerCollider(plr->position, plrFrame));
		if (dmg != 0) {
			plr->hp -= dmg;
			if (dmg > 0) { dmgDebounce = clockTimestampMs + DMG_COOLDOWN; }
		}
	}

	return hasHorizontalMovement || hasVerticalMovement;
}

void FollowPlayerWithCamera(Camera2D* camera, Vector2 plrPosition, Rectangle plrFrame) {
	float margin = GetScreenWidth() / 3.0f;
	float plrScreenX = plrPosition.x - camera->target.x;

	if (plrScreenX < margin) {
		camera->target.x = plrPosition.x - margin;
	} else if (plrScreenX + plrFrame.width > GetScreenWidth() - margin) {
		camera->target.x = plrPosition.x + plrFrame.width - (GetScreenWidth() - margin);
	}

	float maxX = fmaxf(worldSize.x - GetScreenWidth(), 0);
	camera->target.x = fminf(fmaxf(camera->target.x, 0), maxX);
}

void SetupWorld(Camera2D* camera, Character* plr, Rectangle plrFrame, Texture2D* background) {
	if (background != NULL) {
		float scale = (float)GetScreenHeight() / background->height;
		worldSize = (Vector2){background->width * scale, GetScreenHeight()};
	} else {
		worldSize = (Vector2){GetScreenWidth(), GetScreenHeight()};
	}

	camera->target = (Vector2){0, 0};
	plr->position.x = fminf(plr->position.x, worldSize.x - plrFrame.width);
	FollowPlayerWithCamera(camera, plr->position, plrFrame);
}

void DrawWorld(Texture2D* background, LoadedAssetsClass* Assets, bool debugInfo) {
	if (background != NULL) {
		Rectangle source = {0, 0, background->width, background->height};
		Rectangle dest = {0, 0, worldSize.x, worldSize.y};
		DrawTexturePro(*background, source, dest, (Vector2){0, 0}, 0, WHITE);
	}

	if (debugInfo) { Assets->DrawHitboxes(Assets); }
}

//void LoadDrawGameScenarios(int scenarioIndex, LoadedAssetsClass* Assets) {
//	for (int i = 0; i < assets->imageCount; i++) {
//		UnloadImage(assets->images[i].image);
//	}
//
//	for (int i = 0; i < assets->textureCount; i++) {
//		UnloadTexture(assets->textures[i].texture);
//	}
//
//	if (assets->images != NULL) { free(assets->images); }
//	if (assets->textures != NULL) { free(assets->textures); }
//	Assets->FreeHitboxes(Assets);
//
//	
//}


Rectangle ArtRect(float artHeight, float x, float y, float width, float height) {
	float scale = GetScreenHeight() / artHeight;
	return (Rectangle){x * scale, y * scale, width * scale, height * scale};
}

Rectangle EntranceRect(float x, float y, float width, float height) { return ArtRect(ENTRANCE_ART_HEIGHT, x, y, width, height); }
Rectangle BossRoomRect(float x, float y, float width, float height) { return ArtRect(BOSS_ROOM_ART_HEIGHT, x, y, width, height); }

Texture2D* GetScenarioBackground(ScenarioBackground* list, int count, int scenario) {
	for (int i = 0; i < count; i++) {
		if (list[i].scenario == scenario && list[i].texture.id != 0) { return &list[i].texture; }
	}
	return NULL;
}

Vector2 GetScenarioSpawn(int scenario) {
	if (scenario == SCENARIO_ENTRANCE) { return (Vector2){30, 330}; }
	if (scenario == SCENARIO_BOSS_ROOM) { return (Vector2){20, 285}; }
	return GetCenterOfScreen();
}

Vector2 GetDoorArrival(int scenario) {
	if (scenario == SCENARIO_ENTRANCE) { return (Vector2){2230, 305}; }
	return GetScenarioSpawn(scenario);
}

Vector2 GetBossRoomBossPosition(void) {
	Rectangle r = BossRoomRect(1560, 405, 0, 0);
	return (Vector2){r.x, r.y};
}

Rectangle GetBossRoomExit(void) {
	return BossRoomRect(0, 300, 40, 290);
}

void InitCasinoDoor(Door* door) {
	Door_Init(door, EntranceRect(2535, 400, 64, 120), EntranceRect(2440, 410, 82, 70));
}

void DrawGameScenarios(int scenarioIndex, LoadedAssetsClass* Assets) {
	Assets->FreeHitboxes(Assets);
	Assets->FreeImages(Assets);

	if (scenarioIndex == 0) { // Kard Kassino Garden
		Rectangle hb1 = {0, GetScreenHeight() - 350, 300, 300};
		Assets->NewHitbox(Assets, "KardKassinoGarden_HB1", hb1, true, 0);

		Rectangle hb2 = {0, 0, 150, 150};
		Assets->NewHitbox(Assets, "KardKassinoGarden_HB2", hb2, false, 10);

		Rectangle hb3 = {GetScreenWidth() - 200, 0, 200, 200};
		Assets->NewHitbox(Assets, "KardKassinoGarden_HB3", hb3, false, -25);
	} else if (scenarioIndex == SCENARIO_ENTRANCE) { // Kard Kassino Entrace
		Assets->NewHitbox(Assets, "Entrance_Skyline", EntranceRect(0, 0, 2834, 140), true, 0);
		Assets->NewHitbox(Assets, "Entrance_Fountain", EntranceRect(583, 392, 224, 66), true, 0);
		Assets->NewHitbox(Assets, "Entrance_CarRed", EntranceRect(20, 600, 176, 150), true, 0);
		Assets->NewHitbox(Assets, "Entrance_CarBlue", EntranceRect(226, 645, 173, 105), true, 0);
		Assets->NewHitbox(Assets, "Entrance_CarYellow", EntranceRect(506, 625, 168, 125), true, 0);
		Assets->NewHitbox(Assets, "Entrance_CarGreen", EntranceRect(1066, 625, 173, 125), true, 0);
		Assets->NewHitbox(Assets, "Entrance_Planter1", EntranceRect(1537, 545, 110, 65), true, 0);
		Assets->NewHitbox(Assets, "Entrance_Planter2", EntranceRect(1757, 545, 110, 65), true, 0);
		Assets->NewHitbox(Assets, "Entrance_Planter3", EntranceRect(1977, 545, 110, 65), true, 0);
		Assets->NewHitbox(Assets, "Entrance_Planter4", EntranceRect(2197, 545, 110, 65), true, 0);
		Assets->NewHitbox(Assets, "Entrance_Kassino", EntranceRect(2520, 0, 314, 750), true, 0);

	} else if (scenarioIndex == 2) { // Kard Kassino Lobby

	} else if (scenarioIndex == 3) { // Kard Kassino Tables

	} else if (scenarioIndex == 4) { // Kard Kassino Bar

	} else if (scenarioIndex == 5) { // Kard Kassino Backstage

	} else if (scenarioIndex == 6) { // Kard Kassino 

	} else if (scenarioIndex == 7) { // Kard Kassino 

	} else if (scenarioIndex == SCENARIO_BOSS_ROOM) {
		Assets->NewHitbox(Assets, "BossRoom_TopWall", BossRoomRect(0, 0, 1837, 75), true, 0);
		Assets->NewHitbox(Assets, "BossRoom_TopSlope1", BossRoomRect(0, 75, 520, 40), true, 0);
		Assets->NewHitbox(Assets, "BossRoom_TopSlope2", BossRoomRect(0, 115, 440, 50), true, 0);
		Assets->NewHitbox(Assets, "BossRoom_TopSlope3", BossRoomRect(0, 165, 380, 50), true, 0);
		Assets->NewHitbox(Assets, "BossRoom_TopSlope4", BossRoomRect(0, 215, 300, 50), true, 0);
		Assets->NewHitbox(Assets, "BossRoom_TopSlope5", BossRoomRect(0, 265, 210, 45), true, 0);
		Assets->NewHitbox(Assets, "BossRoom_TopColumn1", BossRoomRect(925, 0, 75, 80), true, 0);
		Assets->NewHitbox(Assets, "BossRoom_TopColumn2", BossRoomRect(1370, 0, 75, 90), true, 0);
		Assets->NewHitbox(Assets, "BossRoom_BottomLeft", BossRoomRect(0, 585, 300, 163), true, 0);
		Assets->NewHitbox(Assets, "BossRoom_BottomColumn1", BossRoomRect(285, 580, 80, 168), true, 0);
		Assets->NewHitbox(Assets, "BossRoom_BottomSlope1", BossRoomRect(360, 625, 60, 123), true, 0);
		Assets->NewHitbox(Assets, "BossRoom_BottomSlope2", BossRoomRect(420, 655, 60, 93), true, 0);
		Assets->NewHitbox(Assets, "BossRoom_BottomSlope3", BossRoomRect(480, 685, 70, 63), true, 0);
		Assets->NewHitbox(Assets, "BossRoom_BottomWall", BossRoomRect(0, 705, 1837, 43), true, 0);
		Assets->NewHitbox(Assets, "BossRoom_BottomColumn2", BossRoomRect(925, 688, 80, 60), true, 0);
		Assets->NewHitbox(Assets, "BossRoom_BottomColumn3", BossRoomRect(1355, 685, 80, 63), true, 0);
		Assets->NewHitbox(Assets, "BossRoom_BottomColumn4", BossRoomRect(1775, 680, 62, 68), true, 0);
		Assets->NewHitbox(Assets, "BossRoom_RightWall", BossRoomRect(1800, 0, 37, 748), true, 0);

	} else if (scenarioIndex == 99) { // Kard Kassino Rooftop Hallway

	} else if (scenarioIndex == 100) { // Kard Kassino Rooftop

	}
}

void GetAnimatedCharVector(Rectangle* animFrame, int* ticksToNextFrame, int ticksPerFrame, int currentMaxX, int nextTextureY) {
	if (animFrame->y != 0) {
		(*ticksToNextFrame)++;

		if ((*ticksToNextFrame) >= ticksPerFrame) {
			(*ticksToNextFrame) = 0;
			animFrame->x++;

			if (animFrame->x >= currentMaxX) {
				animFrame->x = 0;
			}
		}
	}

	if (animFrame->y != nextTextureY) {
		if (animFrame->y == 0) {
			ticksToNextFrame = 0;
			animFrame->x = animFrame->y - 1;
		}

		animFrame->y = nextTextureY;
	}
}



Rectangle GetPlayerBody(Character* plr, Rectangle plrFrame) {
	return (Rectangle){plr->position.x + plrFrame.width * 0.25f, plr->position.y + plrFrame.height * 0.3f, plrFrame.width * 0.5f, plrFrame.height * 0.65f};
}

int GetMinionCount(int scenario) {
	if (scenario == SCENARIO_GARDEN) { return 7; }
	if (scenario == SCENARIO_ENTRANCE) { return 4; }
	if (scenario == SCENARIO_BOSS_ROOM) { return 0; }
	return 5;
}

int GetWinsToUnlockDoor(int scenario) {
	return scenario == SCENARIO_ENTRANCE ? 3 : 0;
}

void StartEncounter(Battle* battle, Encounter* enc, const EnemyDef* def, Camera2D* camera, Character* plr, Rectangle plrFrame) {
	Vector2 plrCenter = {plr->position.x + plrFrame.width / 2, plr->position.y + plrFrame.height / 2};
	Battle_Start(battle, def, plr->texture, (Vector2){plrFrame.width, plrFrame.height});
	Encounter_Start(enc, GetWorldToScreen2D(plrCenter, *camera), def->name);
}

int main() {
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(1280, 720, "ALTERNUDE");
	SearchAndSetResourceDir("resources");

	GameDataClass* GameData = InitGameData();
	int plrFrameAmount = 4;
	int plrAnimsAmount = 10;
	GameData->mainCharacter = GameData->NewCharacter(GameData, GetCenterOfScreen(), 17.5, PLAYER_MAX_HP, LoadTexture("playr.png"), (Vector2){plrFrameAmount, plrAnimsAmount});
	Character* plr = GameData->mainCharacter;
	if (plr == NULL) { return 1; }
	Spirit* plrSpirit = GameData->spirit;

	LoadedAssetsClass* Assets = InitLoadedAssets();
	bool isMoving = false;

	int lastScenario = -1;
	int scenario = START_SCENARIO;
	int plrFrameI = 0;
	Vector2 plrCurrentFrame = {0, 0};
	int plrFrameSpeed = 3;
	Rectangle plrFrame = plr->animFrame;

	ScenarioBackground scenarioBackgrounds[] = {
		{SCENARIO_GARDEN, LoadTexture("kardkassino_garden.png")},
		{SCENARIO_ENTRANCE, LoadTexture("kardkassino_entrance_scene.png")},
		{SCENARIO_BOSS_ROOM, LoadTexture("karddkassino_bossroom.png")},
	};
	int backgroundCount = sizeof(scenarioBackgrounds) / sizeof(scenarioBackgrounds[0]);
	SceneOverlay entranceFountain = SceneOverlay_Load("kardkassino_entrance_fountain.png", (Vector2){579, 210}, 425);
	Texture2D* background = NULL;
	Camera2D camera = {0};
	camera.zoom = 1.0f;

	Battle battle = {0};
	Encounter encounter = {0};

	Score score;
	Score_Init(&score, "highscore.txt");
	MinionField minions = {0};
	Vector2 plrFacing = {0, 1};
	bool scoreSavedOnDeath = false;
	Progression progress;
	Progression_Reset(&progress, 0);
	Door casinoDoor;
	InitCasinoDoor(&casinoDoor);
	bool viaDoor = false;

	const EnemyDef* cutsceneBattle = NULL;
	bool bossCutsceneSeen = false;

	while (!WindowShouldClose()) {
		WaitTime(1.0 / 60.0);
		clockTimestampMs += (1.0 / 60.0);

		if (IsKeyPressed(KEY_P)) {
			GameData->debugMode = !GameData->debugMode;
		}

		Encounter_Update(&encounter, 1.0f / 60.0f);
		bool inIntro = Encounter_IsActive(&encounter);

		Cutscene_Update(1.0f / 60.0f);
		bool inCutscene = Cutscene_IsActive();
		if (!inCutscene && cutsceneBattle != NULL) {
			Battle_Start(&battle, cutsceneBattle, plr->texture, (Vector2){plrFrame.width, plrFrame.height});
			cutsceneBattle = NULL;
		}

		bool inBattle = Battle_IsActive(&battle) && !inIntro;
		StageContext stage = {&minions, &score};

		if (inBattle) {
			Battle_Update(&battle, 1.0f / 60.0f, &plr->hp);
			if (!Battle_IsActive(&battle)) { Progression_EndFight(&progress, Battle_Outcome(&battle), stage); }
			inBattle = Battle_IsActive(&battle);
		}

		if (!inBattle && !inIntro && !inCutscene) {
			if (IsKeyPressed(KEY_Q)) { scenario -= 1; }
			if (IsKeyPressed(KEY_E)) { scenario += 1; printf("NEXT"); }
		}

		if (IsKeyPressed(KEY_SPACE) && !inIntro && !inBattle && !inCutscene) {
			GameData->spiritMode = !GameData->spiritMode;

			if (GameData->spiritMode) {
				Vector2 center = GetCenterOfScreen();
				plrSpirit->position.x = center.x;
				plrSpirit->position.y = center.y;
			}
		}

		BeginDrawing();
		ClearBackground(DARKGREEN);

		// reinicia a luta
		if ((IsKeyPressed(KEY_R) && !inBattle && !inIntro && !inCutscene) || Battle_ConsumeRestart(&battle)) {
			plr->hp = PLAYER_MAX_HP;
			Score_Save(&score);
			Score_Reset(&score);
			scoreSavedOnDeath = false;
			if (scenario == SCENARIO_BOSS_ROOM) { scenario = SCENARIO_ENTRANCE; }
			lastScenario = -1;
			viaDoor = false;
			inBattle = false;
			inIntro = false;
		}

		if (lastScenario != scenario) {
			lastScenario = scenario;
			DrawGameScenarios(scenario, Assets);

			background = GetScenarioBackground(scenarioBackgrounds, backgroundCount, scenario);
			plr->position = viaDoor ? GetDoorArrival(scenario) : GetScenarioSpawn(scenario);
			SetupWorld(&camera, plr, plrFrame, background);
			Minions_Spawn(&minions, GetMinionCount(scenario), worldSize, GetPlayerBody(plr, plrFrame), Assets);
			if (!viaDoor) {
				Progression_Reset(&progress, GetWinsToUnlockDoor(scenario));
				InitCasinoDoor(&casinoDoor);
			}
			Progression_SetInBossRoom(&progress, scenario == SCENARIO_BOSS_ROOM);
			if (scenario == SCENARIO_BOSS_ROOM && !progress.cleared) { Minions_SpawnBoss(&minions, GetBossRoomBossPosition()); }
			Battle_Stop(&battle);
			Encounter_Stop(&encounter);
			Cutscene_Stop();
			cutsceneBattle = NULL;
			viaDoor = false;
		}



		if (inCutscene) {
			Cutscene_Draw();
		} else if (inBattle) {
			Battle_Draw(&battle, plr->hp);
		} else if (GameData->spiritMode) {
			BeginMode2D(camera);
			DrawWorld(background, Assets, GameData->debugMode);
			EndMode2D();

			MoveSpirit(plrSpirit);

			DrawBattleBackground(GetBattleSquareSize());
			DrawRectangle(plrSpirit->position.x - (playerSpiritSize / 2), plrSpirit->position.y - (playerSpiritSize / 2), playerSpiritSize, playerSpiritSize, RED);
		} else {
			bool hasMovement = (plr->hp > 0 && !inIntro) ? MovePlr(plr, plrFrame, &plrCurrentFrame, Assets) : false;

			if (hasMovement) {
				plrFrameI++;
				if (plrFrameI >= plrFrameSpeed) {
					plrFrameI = 0;
					plrCurrentFrame.x++;
					if (plrCurrentFrame.x >= plrFrameAmount) {
						plrCurrentFrame.x = 0;
					}
				}
			} else if (isMoving != hasMovement) {
				plrFrameI = 0;
				plrCurrentFrame.x = plrCurrentFrame.y - 1;
				plrCurrentFrame.y = 0;
			}

			isMoving = hasMovement;

			if (!inIntro) {
				if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) { plrFacing = (Vector2){-1, 0}; }
				if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) { plrFacing = (Vector2){ 1, 0}; }
				if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) { plrFacing = (Vector2){ 0,-1}; }
				if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) { plrFacing = (Vector2){ 0, 1}; }
			}

			// [J], [K] ou clique esquerdo = bater
			bool attackPressed = !inIntro && (IsKeyPressed(KEY_J) || IsKeyPressed(KEY_K) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT));
			Rectangle plrBody = GetPlayerBody(plr, plrFrame);
			if (!inIntro) {
				MinionInput input = {plrBody, plrFacing, attackPressed, plr->hp > 0, plr->hp > 0 && Progression_CanEncounter(&progress)};
				int touched = Minions_Update(&minions, 1.0f / 60.0f, input, &score, worldSize, Assets);
				Rectangle feet = GetPlayerCollider(plr->position, plrFrame);
				if (touched >= 0) {
					const EnemyDef* def = Enemy_Get(Minions_EnemyOf(&minions, touched));
					Progression_BeginFight(&progress, touched);
					if (def->isBoss && !bossCutsceneSeen) {
						bossCutsceneSeen = true;
						cutsceneBattle = def;
						Cutscene_StartBoss(plr->texture, plrFrame);
					} else {
						StartEncounter(&battle, &encounter, def, &camera, plr, plrFrame);
					}
				} else if (scenario == SCENARIO_ENTRANCE) {
					Door_Update(&casinoDoor, 1.0f / 60.0f, progress.doorUnlocked, feet);
					if (Door_IsEntering(&casinoDoor, feet)) { scenario = SCENARIO_BOSS_ROOM; viaDoor = true; }
				} else if (scenario == SCENARIO_BOSS_ROOM && CheckCollisionRecs(GetBossRoomExit(), feet)) {
					scenario = SCENARIO_ENTRANCE;
					viaDoor = true;
				}
				Progression_Update(&progress, 1.0f / 60.0f);
			}

			if (plr->hp <= 0 && !scoreSavedOnDeath) {
				Score_Save(&score);
				scoreSavedOnDeath = true;
			}

			FollowPlayerWithCamera(&camera, plr->position, plrFrame);

			BeginMode2D(camera);
			DrawWorld(background, Assets, GameData->debugMode);
			if (scenario == SCENARIO_ENTRANCE) {
				Door_Draw(&casinoDoor, TextFormat("Trancada! Venca mais %d inimigo(s)", Progression_WinsLeft(&progress)));
			}
			Minions_Draw(&minions);
			DrawTextureRec(plr->texture, (Rectangle){plrFrame.width * plrCurrentFrame.x, plrFrame.height * plrCurrentFrame.y, plrFrame.width, plrFrame.height}, plr->position, WHITE);
			if (scenario == SCENARIO_ENTRANCE) {
				Rectangle feet = GetPlayerCollider(plr->position, plrFrame);
				SceneOverlay_DrawIfInFront(&entranceFountain, feet.y + feet.height, GetScreenHeight() / ENTRANCE_ART_HEIGHT);
			}
			if (GameData->debugMode) { DrawRectangleLinesEx(plrBody, 2, YELLOW); }
			Score_DrawPopups(&score);
			EndMode2D();

			if (plr->hp <= 0) {
				const char* msg = "VOCE CAIU! [R] para recomecar";
				DrawText(msg, GetScreenWidth() / 2 - MeasureText(msg, 40) / 2, GetScreenHeight() / 2 - 20, 40, RED);
			}
		}

		Score_Update(&score, 1.0f / 60.0f);
		if (!inBattle && !inIntro && !inCutscene) {
			Score_DrawHud(&score);
			Progression_DrawHud(&progress);
		}

		if (inIntro) { Encounter_Draw(&encounter); }

		if (!inCutscene) { DrawText("Press [P] to show/hide debug info!", 15, 20, 30, WHITE); }
		if (GameData->debugMode && !inCutscene) {
			DrawText(TextFormat("HP: %d", plr->hp), 15, 70, 20, RED);
			DrawText(TextFormat("Loaded Hitboxes: %d", Assets->GetHitboxAmount(Assets)), 15, 90, 20, BLUE);
			DrawText(TextFormat("Scenario: %d", scenario), 15, 110, 20, LIGHTGRAY);

			char* spiritStateStr = (GameData->spiritMode == true) ? "ON" : "OFF";
			DrawText(TextFormat("Spirit Mode: %s ([SPACE] to toggle!)", spiritStateStr), 15, 50, 20, LIGHTGRAY);
			if (!GameData->spiritMode) { DrawText(TextFormat("Player Sprite State - (X:%.0f, Y:%.0f)", plrCurrentFrame.x, plrCurrentFrame.y), 15, GetScreenHeight() - 40, 20, LIGHTGRAY); }
		}
		EndDrawing();
	}

	Score_Save(&score);
	for (int i = 0; i < backgroundCount; i++) { UnloadTexture(scenarioBackgrounds[i].texture); }
	SceneOverlay_Unload(&entranceFountain);
	Cutscene_Unload();
	GameData->UnloadCharacter(GameData, plr);
	CloseWindow();

	Assets->FreeHitboxes(Assets);
	Assets->FreeImages(Assets);
	if (Assets != NULL) { free(Assets); }
	return 0;
}
