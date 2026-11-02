#include <raylib.h>
#include <resource_dir.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <classes.h>
#include <time.h>



typedef struct {
	Vector2 position;
	float movespeed;
} Spirit;

typedef struct {
	Vector2 position;
	float movespeed;
	int hp;
	Spirit spirit;
} Character;



const bool DEBUG_MODE = true;
const float DMG_COOLDOWN = 0.8;

int playerSpiritSize = 40;
float dmgDebounce = 0;
float clockTimestampMs = 0;
Vector2 worldSize = {0, 0};

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
		Rectangle hb1 = {0, 0, (GetScreenWidth() /1.5), (GetScreenHeight()/2) - 140};
		Assets->NewHitbox(Assets, "arar", hb1, true, 0);

		Rectangle hb2 = {0, (GetScreenHeight() - ((GetScreenHeight()/2) - 140)), (GetScreenWidth() /1.5), (GetScreenHeight()/2) - 140};
		Assets->NewHitbox(Assets, "arare", hb2, true, 0);

		Rectangle hb3 = {GetScreenWidth() / 2, (GetScreenHeight()/2) - 140, 35, 280};
		Assets->NewHitbox(Assets, "healer", hb3, false, -30);

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



int main() {
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(1280, 720, "ALTERNUDE");
	SearchAndSetResourceDir("resources");

	Spirit plrSpirit = {GetCenterOfScreen(), 8.5};
	Character* plr = (Character*) malloc(sizeof(Character));
	plr->hp = 100;
	plr->movespeed = 17.5;
	plr->spirit = plrSpirit;
	plr->position = GetCenterOfScreen();

	LoadedAssetsClass* Assets = InitLoadedAssets();
	Assets->plrTexture = LoadTexture("playr.png");
	if (Assets->plrTexture.id == 0) {
		TraceLog(LOG_ERROR, "Failed to load playr.png (is the resources folder reachable?)");
		free(plr);
		free(Assets);
		CloseWindow();
		return 1;
	}

	bool spiritMode = false;
	bool isMoving = false;
	bool debugInfo = false;

	int lastScenario = -1;
	int scenario = 0;
	int plrFrameI = 0;
	Vector2 plrCurrentFrame = {0, 0};
	int plrFrameAmount = 4;
	int plrFrameSpeed = 3;
	Rectangle plrFrame = {0, 0, Assets->plrTexture.width / plrFrameAmount, Assets->plrTexture.height / 5};

	Texture2D gardenBackground = LoadTexture("kardkassino_garden.png");
	Texture2D* background = NULL;
	Camera2D camera = {0};
	camera.zoom = 1.0f;


	while (!WindowShouldClose()) {
		WaitTime(1.0 / 60.0);
		clockTimestampMs += (1.0 / 60.0);

		if (IsKeyPressed(KEY_P)) {
			debugInfo = !debugInfo;
		}

		if (IsKeyPressed(KEY_Q)) { scenario -= 1; }
		if (IsKeyPressed(KEY_E)) { scenario += 1; printf("NEXT"); }

		if (IsKeyPressed(KEY_SPACE)) {
			spiritMode = !spiritMode;

			if (spiritMode) {
				Vector2 center = GetCenterOfScreen();
				plrSpirit.position.x = center.x;
				plrSpirit.position.y = center.y;
			}
		}

		BeginDrawing();
		ClearBackground(DARKGREEN);

		if (lastScenario != scenario) {
			lastScenario = scenario;
			DrawGameScenarios(scenario, Assets);

			background = (scenario == 0 && gardenBackground.id != 0) ? &gardenBackground : NULL;
			SetupWorld(&camera, plr, plrFrame, background);
		}



		if (spiritMode) {
			BeginMode2D(camera);
			DrawWorld(background, Assets, debugInfo);
			EndMode2D();

			MoveSpirit(&plrSpirit);

			DrawBattleBackground(GetBattleSquareSize());
			DrawRectangle(plrSpirit.position.x - (playerSpiritSize / 2), plrSpirit.position.y - (playerSpiritSize / 2), playerSpiritSize, playerSpiritSize, RED);
		} else {
			bool hasMovement = MovePlr(plr, plrFrame, &plrCurrentFrame, Assets);

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

			FollowPlayerWithCamera(&camera, plr->position, plrFrame);

			BeginMode2D(camera);
			DrawWorld(background, Assets, debugInfo);
			DrawTextureRec(Assets->plrTexture, (Rectangle){plrFrame.width * plrCurrentFrame.x, plrFrame.height * plrCurrentFrame.y, plrFrame.width, plrFrame.height}, plr->position, WHITE);
			EndMode2D();
		}

		DrawText("Press [P] to show/hide debug info!", 15, 20, 30, WHITE);
		if (debugInfo) {
			DrawText(TextFormat("HP: %d", plr->hp), 15, 70, 20, RED);
			DrawText(TextFormat("Loaded Hitboxes: %d", Assets->GetHitboxAmount(Assets)), 15, 90, 20, BLUE);
			DrawText(TextFormat("Scenario: %d", scenario), 15, 110, 20, LIGHTGRAY);

			char* spiritStateStr = (spiritMode == true) ? "ON" : "OFF";
			DrawText(TextFormat("Spirit Mode: %s ([SPACE] to toggle!)", spiritStateStr), 15, 50, 20, LIGHTGRAY);
			if (spiritMode) { DrawText(TextFormat("Player Sprite State - (X:%.0f, Y:%.0f)", plrCurrentFrame.x, plrCurrentFrame.y), 15, GetScreenHeight() - 40, 20, LIGHTGRAY); }
		}
		EndDrawing();
	}

	UnloadTexture(gardenBackground);
	UnloadTexture(Assets->plrTexture);
	CloseWindow();

	Assets->FreeHitboxes(Assets);
	Assets->FreeImages(Assets);
	if (Assets != NULL) { free(Assets); }
	return 0;
}	
