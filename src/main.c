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
#include <time.h>







const bool DEBUG_MODE = true;
const float DMG_COOLDOWN = 0.8;

int playerSpiritSize = 40;
float dmgDebounce = 0;
float clockTimestampMs = 0;
Vector2 worldSize = {0, 0};
const float SCENE_ART_HEIGHT = 750.0f;
const int START_SCENARIO = 1;

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

bool MovePlr(Character* plr, Rectangle plrTextureRect, Vector2* plrCurrentFrame, LoadedAssetsClass* Assets) {
	float speed       = plr->movespeed;
	float sqrtSpeed   = speed / sqrt(2);

	float leftLimit   = 0;
	float topLimit    = 0;
	float rightLimit  = worldSize.x - plrTextureRect.width;
	float bottomLimit = worldSize.y - plrTextureRect.height;

	bool hasVerticalMovement   = IsKeyDown(KEY_UP)   || IsKeyDown(KEY_W) || IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S);
	bool hasHorizontalMovement = IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A) || IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
	bool moved = false;
	Rectangle futurePos = {0, 0, 0, 0};

	float verticalStep = hasHorizontalMovement ? sqrtSpeed : speed;
	float horizontalStep = hasVerticalMovement ? sqrtSpeed : speed;


	if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
		float newY = fminf(plr->position.y + verticalStep, bottomLimit);
		futurePos = (Rectangle){plr->position.x, newY, plrTextureRect.width, plrTextureRect.height};
		moved = true;

		if (!Assets->CheckColisions(Assets, futurePos)) {
			plr->position.y = newY;
			plrCurrentFrame->y = 1;
		}
	}

	if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
		float newY = fmaxf(plr->position.y - verticalStep, topLimit);
		futurePos = (Rectangle){plr->position.x, newY, plrTextureRect.width, plrTextureRect.height};
		moved = true;

		if (!Assets->CheckColisions(Assets, futurePos)) {
			plr->position.y = newY;
			plrCurrentFrame->y = 3;
		}
	}

	if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
		float newX = fmaxf(plr->position.x - horizontalStep, leftLimit);
		futurePos = (Rectangle){newX, plr->position.y, plrTextureRect.width, plrTextureRect.height};
		moved = true;

		if (!Assets->CheckColisions(Assets, futurePos)) {
			plr->position.x = newX;
			plrCurrentFrame->y = 2;
		}
	}

	if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
		float newX = fminf(plr->position.x + horizontalStep, rightLimit);
		futurePos = (Rectangle){newX, plr->position.y, plrTextureRect.width, plrTextureRect.height};
		moved = true;

		if (!Assets->CheckColisions(Assets, futurePos)) {
			plr->position.x = newX;
			plrCurrentFrame->y = 4;
		}
	}

	//printf("time: %d\ndb: %d\n", sec, dmgDebounce);
	if (dmgDebounce - clockTimestampMs <= 0 && moved) {
		float dmg = Assets->CheckDamageColisions(Assets, futurePos);
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


Rectangle SceneRect(float x, float y, float width, float height) {
	float scale = GetScreenHeight() / SCENE_ART_HEIGHT;
	return (Rectangle){x * scale, y * scale, width * scale, height * scale};
}

Texture2D* GetScenarioBackground(Texture2D* backgrounds, int count, int scenario) {
	if (scenario < 0 || scenario >= count || backgrounds[scenario].id == 0) { return NULL; }
	return &backgrounds[scenario];
}

Vector2 GetScenarioSpawn(int scenario) {
	if (scenario == 1) { return (Vector2){30, 330}; }
	return GetCenterOfScreen();
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
	} else if (scenarioIndex == 1) { // Kard Kassino Entrace
		Assets->NewHitbox(Assets, "Entrance_Skyline", SceneRect(0, 0, 2834, 140), true, 0);
		Assets->NewHitbox(Assets, "Entrance_Fountain", SceneRect(575, 370, 250, 100), true, 0);
		Assets->NewHitbox(Assets, "Entrance_CarRed", SceneRect(20, 600, 176, 150), true, 0);
		Assets->NewHitbox(Assets, "Entrance_CarBlue", SceneRect(226, 645, 173, 105), true, 0);
		Assets->NewHitbox(Assets, "Entrance_CarYellow", SceneRect(506, 625, 168, 125), true, 0);
		Assets->NewHitbox(Assets, "Entrance_CarGreen", SceneRect(1066, 625, 173, 125), true, 0);
		Assets->NewHitbox(Assets, "Entrance_Planter1", SceneRect(1537, 545, 110, 65), true, 0);
		Assets->NewHitbox(Assets, "Entrance_Planter2", SceneRect(1757, 545, 110, 65), true, 0);
		Assets->NewHitbox(Assets, "Entrance_Planter3", SceneRect(1977, 545, 110, 65), true, 0);
		Assets->NewHitbox(Assets, "Entrance_Planter4", SceneRect(2197, 545, 110, 65), true, 0);
		Assets->NewHitbox(Assets, "Entrance_Kassino", SceneRect(2520, 0, 314, 750), true, 0);

	} else if (scenarioIndex == 2) { // Kard Kassino Lobby

	} else if (scenarioIndex == 3) { // Kard Kassino Tables

	} else if (scenarioIndex == 4) { // Kard Kassino Bar

	} else if (scenarioIndex == 5) { // Kard Kassino Backstage

	} else if (scenarioIndex == 6) { // Kard Kassino 

	} else if (scenarioIndex == 7) { // Kard Kassino 

	} else if (scenarioIndex == 8) { // Kard Kassino 

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
	if (scenario == 0) { return 7; }
	if (scenario == 1) { return 4; }
	return 5;
}

int GetWinsToBoss(int scenario) {
	return (scenario == 0 || scenario == 1) ? 3 : 0;
}

void StartEncounter(Battle* battle, Encounter* enc, const EnemyDef* def, Camera2D* camera, Character* plr, Rectangle plrFrame, Rectangle battlePlrSrc) {
	Vector2 plrCenter = {plr->position.x + plrFrame.width / 2, plr->position.y + plrFrame.height / 2};
	Battle_Start(battle, def, plr->texture, battlePlrSrc);
	Encounter_Start(enc, GetWorldToScreen2D(plrCenter, *camera), def->name);
}

int main() {
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(1280, 720, "ALTERNUDE");
	SearchAndSetResourceDir("resources");

	GameDataClass* GameData = InitGameData();
	int plrFrameAmount = 4;
	int plrAnimsAmount = 5;
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

	Texture2D scenarioBackgrounds[] = {LoadTexture("kardkassino_garden.png"), LoadTexture("kardkassino_entrance_scene.png")};
	int backgroundCount = sizeof(scenarioBackgrounds) / sizeof(scenarioBackgrounds[0]);
	Texture2D* background = NULL;
	Camera2D camera = {0};
	camera.zoom = 1.0f;

	Battle battle = {0};
	Encounter encounter = {0};
	Rectangle battlePlrSrc = {plrFrame.width * 3, 0, plrFrame.width, plrFrame.height};

	Score score;
	Score_Init(&score, "highscore.txt");
	MinionField minions = {0};
	Vector2 plrFacing = {0, 1};
	bool scoreSavedOnDeath = false;
	Progression progress;
	Progression_Reset(&progress, 0);

	while (!WindowShouldClose()) {
		WaitTime(1.0 / 60.0);
		clockTimestampMs += (1.0 / 60.0);

		if (IsKeyPressed(KEY_P)) {
			GameData->debugMode = !GameData->debugMode;
		}

		Encounter_Update(&encounter, 1.0f / 60.0f);
		bool inIntro = Encounter_IsActive(&encounter);
		bool inBattle = Battle_IsActive(&battle) && !inIntro;
		StageContext stage = {&minions, &score, worldSize, GetPlayerBody(plr, plrFrame), Assets};

		if (inBattle) {
			Battle_Update(&battle, 1.0f / 60.0f, &plr->hp);
			if (!Battle_IsActive(&battle)) { Progression_EndFight(&progress, Battle_Outcome(&battle), stage); }
			inBattle = Battle_IsActive(&battle);
		}

		if (!inBattle && !inIntro) {
			if (IsKeyPressed(KEY_Q)) { scenario -= 1; }
			if (IsKeyPressed(KEY_E)) { scenario += 1; printf("NEXT"); }
		}

		if (IsKeyPressed(KEY_SPACE) && !inIntro && !inBattle) {
			GameData->spiritMode = !GameData->spiritMode;

			if (GameData->spiritMode) {
				Vector2 center = GetCenterOfScreen();
				plrSpirit->position.x = center.x;
				plrSpirit->position.y = center.y;
			}
		}

		BeginDrawing();
		ClearBackground(DARKGREEN);

		if (lastScenario != scenario) {
			lastScenario = scenario;
			DrawGameScenarios(scenario, Assets);

			background = GetScenarioBackground(scenarioBackgrounds, backgroundCount, scenario);
			plr->position = GetScenarioSpawn(scenario);
			SetupWorld(&camera, plr, plrFrame, background);
			Minions_Spawn(&minions, GetMinionCount(scenario), worldSize, GetPlayerBody(plr, plrFrame), Assets);
			Progression_Reset(&progress, GetWinsToBoss(scenario));
			Battle_Stop(&battle);
			Encounter_Stop(&encounter);
		}

		// reinicia a luta
		if ((IsKeyPressed(KEY_R) && !inBattle && !inIntro) || Battle_ConsumeRestart(&battle)) {
			plr->hp = PLAYER_MAX_HP;
			plr->position = GetScenarioSpawn(scenario);
			Score_Save(&score);
			Score_Reset(&score);
			scoreSavedOnDeath = false;
			Minions_Spawn(&minions, GetMinionCount(scenario), worldSize, GetPlayerBody(plr, plrFrame), Assets);
			Progression_Reset(&progress, GetWinsToBoss(scenario));
			Battle_Stop(&battle);
			Encounter_Stop(&encounter);
			inBattle = false;
			inIntro = false;
		}



		if (inBattle) {
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
				if (touched >= 0) {
					Progression_BeginFight(&progress, touched);
					StartEncounter(&battle, &encounter, Enemy_Get(Minions_EnemyOf(&minions, touched)), &camera, plr, plrFrame, battlePlrSrc);
				}
				Progression_Update(&progress, 1.0f / 60.0f, stage);
			}

			if (plr->hp <= 0 && !scoreSavedOnDeath) {
				Score_Save(&score);
				scoreSavedOnDeath = true;
			}

			FollowPlayerWithCamera(&camera, plr->position, plrFrame);

			BeginMode2D(camera);
			DrawWorld(background, Assets, GameData->debugMode);
			Minions_Draw(&minions);
			DrawTextureRec(plr->texture, (Rectangle){plrFrame.width * plrCurrentFrame.x, plrFrame.height * plrCurrentFrame.y, plrFrame.width, plrFrame.height}, plr->position, WHITE);
			if (GameData->debugMode) { DrawRectangleLinesEx(plrBody, 2, YELLOW); }
			Score_DrawPopups(&score);
			EndMode2D();

			if (plr->hp <= 0) {
				const char* msg = "VOCE CAIU! [R] para recomecar";
				DrawText(msg, GetScreenWidth() / 2 - MeasureText(msg, 40) / 2, GetScreenHeight() / 2 - 20, 40, RED);
			}
		}

		Score_Update(&score, 1.0f / 60.0f);
		if (!inBattle && !inIntro) {
			Score_DrawHud(&score);
			Progression_DrawHud(&progress);
		}

		if (inIntro) { Encounter_Draw(&encounter); }

		DrawText("Press [P] to show/hide debug info!", 15, 20, 30, WHITE);
		if (GameData->debugMode) {
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
	for (int i = 0; i < backgroundCount; i++) { UnloadTexture(scenarioBackgrounds[i]); }
	GameData->UnloadCharacter(GameData, plr);
	CloseWindow();

	Assets->FreeHitboxes(Assets);
	Assets->FreeImages(Assets);
	if (Assets != NULL) { free(Assets); }
	return 0;
}
