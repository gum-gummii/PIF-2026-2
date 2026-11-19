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
#include <scenes.h>
#include <cutscene.h>
#include <time.h>







const bool DEBUG_MODE = false;
const float DMG_COOLDOWN = 0.8;

int playerSpiritSize = 40;
float dmgDebounce = 0;
float clockTimestampMs = 0;
Vector2 worldSize = {0, 0};
const SceneId START_SCENE = SCENE_PARKING;

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

void PlaceFeetAt(Character* plr, Rectangle plrFrame, Vector2 feet) {
	plr->position.x = feet.x - plrFrame.width * 0.51f;
	plr->position.y = feet.y - plrFrame.height * 0.885f;
}

bool IsLockOpen(ExitLock lock, const Progression* p) {
	if (lock == LOCK_CASINO) { return p->casinoUnlocked; }
	if (lock == LOCK_BOSS_KEY) { return p->hasKey; }
	return true;
}

const char* LockHint(ExitLock lock, const Progression* p) {
	if (lock == LOCK_CASINO) { return TextFormat("Trancada! Venca mais %d inimigo(s)", Progression_WinsLeft(p)); }
	return "Trancado! Precisa da CHAVE";
}

int FindPassableExit(const SceneDef* def, const Door doors[MAX_SCENE_EXITS], Rectangle feet) {
	for (int i = 0; i < def->exitCount; i++) {
		const SceneExit* e = &def->exits[i];
		if (!CheckCollisionRecs(Scene_RectToWorld(def, e->trigger), feet)) { continue; }
		if (e->lock == LOCK_NONE || Door_IsOpen(&doors[i])) { return i; }
	}
	return -1;
}

void UpdateSceneDoors(const SceneDef* def, Door doors[MAX_SCENE_EXITS], const Progression* p, Rectangle feet) {
	for (int i = 0; i < def->exitCount; i++) {
		Door_Update(&doors[i], 1.0f / 60.0f, IsLockOpen(def->exits[i].lock, p), feet);
	}
}

void DrawSceneDoors(const SceneDef* def, const Door doors[MAX_SCENE_EXITS], const Progression* p) {
	for (int i = 0; i < def->exitCount; i++) {
		ExitLock lock = def->exits[i].lock;
		if (lock != LOCK_NONE) { Door_Draw(&doors[i], LockHint(lock, p)); }
	}
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

	int lastScene = -1;
	SceneId scene = START_SCENE;
	int plrFrameI = 0;
	Vector2 plrCurrentFrame = {0, 0};
	int plrFrameSpeed = 3;
	Rectangle plrFrame = plr->animFrame;

	Scenes_LoadAssets();
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
	Progression_Reset(&progress);
	Door doors[MAX_SCENE_EXITS] = {0};
	bool viaExit = false;
	Vector2 arrivalFeet = {0, 0};
	const EnemyDef* bossDef = Enemy_Get(ENEMY_DRAGAO);

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
			if (IsKeyPressed(KEY_Q) && scene > 0) { scene--; }
			if (IsKeyPressed(KEY_E) && scene < SCENE_COUNT - 1) { scene++; }
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
			lastScene = -1;
			viaExit = false;
			inBattle = false;
			inIntro = false;
		}

		if (lastScene != (int)scene) {
			lastScene = scene;
			const SceneDef* sceneDef = Scene_Get(scene);
			Scene_LoadHitboxes(sceneDef, Assets);
			background = Scene_Background(scene);
			PlaceFeetAt(plr, plrFrame, Scene_PointToWorld(sceneDef, viaExit ? arrivalFeet : sceneDef->spawn));
			SetupWorld(&camera, plr, plrFrame, background);
			Minions_Spawn(&minions, sceneDef->minionCount, worldSize, GetPlayerBody(plr, plrFrame), Assets);
			if (sceneDef->hasDemon && !progress.hasKey) { Minions_SpawnDemon(&minions, Scene_PointToWorld(sceneDef, sceneDef->demonPos)); }
			Scene_InitDoors(sceneDef, doors);
			Progression_SetInBossRoom(&progress, sceneDef->isBossRoom);
			Battle_Stop(&battle);
			Encounter_Stop(&encounter);
			Cutscene_Stop();
			cutsceneBattle = NULL;
			viaExit = false;

			if (sceneDef->isBossRoom && !progress.cleared) {
				Progression_BeginFight(&progress, -1, bossDef);
				if (!bossCutsceneSeen) {
					bossCutsceneSeen = true;
					cutsceneBattle = bossDef;
					Cutscene_StartBoss(plr->texture, plrFrame);
				} else {
					StartEncounter(&battle, &encounter, bossDef, &camera, plr, plrFrame);
				}
			}
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
					Progression_BeginFight(&progress, touched, def);
					StartEncounter(&battle, &encounter, def, &camera, plr, plrFrame);
				} else {
					const SceneDef* sceneDef = Scene_Get(scene);
					UpdateSceneDoors(sceneDef, doors, &progress, feet);
					int exitIndex = FindPassableExit(sceneDef, doors, feet);
					if (exitIndex >= 0) {
						arrivalFeet = sceneDef->exits[exitIndex].arrival;
						scene = sceneDef->exits[exitIndex].target;
						viaExit = true;
					}
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
			DrawSceneDoors(Scene_Get(scene), doors, &progress);
			Minions_Draw(&minions);
			DrawTextureRec(plr->texture, (Rectangle){plrFrame.width * plrCurrentFrame.x, plrFrame.height * plrCurrentFrame.y, plrFrame.width, plrFrame.height}, plr->position, WHITE);
			Rectangle plrFeet = GetPlayerCollider(plr->position, plrFrame);
			Scene_DrawOverlay(scene, plrFeet.y + plrFeet.height);
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
			DrawText(TextFormat("Scenario: %d", scene), 15, 110, 20, LIGHTGRAY);

			char* spiritStateStr = (GameData->spiritMode == true) ? "ON" : "OFF";
			DrawText(TextFormat("Spirit Mode: %s ([SPACE] to toggle!)", spiritStateStr), 15, 50, 20, LIGHTGRAY);
			if (!GameData->spiritMode) { DrawText(TextFormat("Player Sprite State - (X:%.0f, Y:%.0f)", plrCurrentFrame.x, plrCurrentFrame.y), 15, GetScreenHeight() - 40, 20, LIGHTGRAY); }
		}
		EndDrawing();
	}

	Score_Save(&score);
	Scenes_UnloadAssets();
	Cutscene_Unload();
	GameData->UnloadCharacter(GameData, plr);
	CloseWindow();

	Assets->FreeHitboxes(Assets);
	Assets->FreeImages(Assets);
	if (Assets != NULL) { free(Assets); }
	return 0;
}
