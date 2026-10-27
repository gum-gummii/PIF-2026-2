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

int GetBattleSquareSize() { return GetScreenHeight() - 50; }
int GetBattleBorderSize() { return (int)round(GetBattleSquareSize() / 50); }
Vector2 GetCenterOfScreen() { return (Vector2){GetScreenWidth() / 2, GetScreenHeight() / 2}; }



void DrawBattleBackground(int squareSize) {
	float x = (GetScreenWidth() / 2) - (squareSize / 2);
	float y = (GetScreenHeight() / 2) - (squareSize / 2);

	Rectangle box = {x, y, squareSize, squareSize};
	Color bgColor = {0, 0, 0, 100};

	DrawRectangle(x, y, squareSize, squareSize, bgColor);
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

	if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
		if (plrSpirit->position.x > leftLimit) {
			plrSpirit->position.x -= hasVerticalMovement ? sqrtSpeed : speed;
		} else {
			plrSpirit->position.x = leftLimit;
		}
	}

	if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
		if (plrSpirit->position.x < rightLimit) {
			plrSpirit->position.x += hasVerticalMovement ? sqrtSpeed : speed;
		} else {
			plrSpirit->position.x = rightLimit;
		}
	}

	if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
		if (plrSpirit->position.y > topLimit) {
			plrSpirit->position.y -= hasHorizontalMovement ? sqrtSpeed : speed;
		} else {
			plrSpirit->position.y = topLimit;
		}
	}

	if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
		if (plrSpirit->position.y < bottomLimit) {
			plrSpirit->position.y += hasHorizontalMovement ? sqrtSpeed : speed;
		} else {
			plrSpirit->position.y = bottomLimit;
		}
	}
}

bool MovePlr(Character* plr, Rectangle plrTextureRect, Vector2* plrCurrentFrame, LoadedAssetsClass* Assets) {
	float speed       = plr->movespeed;
	float sqrtSpeed   = speed / sqrt(2);

	float leftLimit   = 0;
	float topLimit    = 0;
	float rightLimit  = GetScreenWidth() - plrTextureRect.width;
	float bottomLimit = GetScreenHeight() - plrTextureRect.height;

	bool hasVerticalMovement   = IsKeyDown(KEY_UP)   || IsKeyDown(KEY_W) || IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S);
	bool hasHorizontalMovement = IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A) || IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
	bool moved = false;
	Rectangle futurePos = {0, 0, 0, 0};

	float verticalStep = hasHorizontalMovement ? sqrtSpeed : speed;
	float horizontalStep = hasVerticalMovement ? sqrtSpeed : speed;


	if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
		futurePos = (Rectangle){plr->position.x, plr->position.y + verticalStep, plrTextureRect.width, plrTextureRect.height};
		moved = true;
		
		if (!Assets->CheckColisions(Assets, futurePos)) {
			if (plr->position.y >= bottomLimit) {
				plr->position.y = bottomLimit;
			} else {
				plr->position.y += verticalStep;
				plrCurrentFrame->y = 1;
			}
		}
	}

	if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
		futurePos = (Rectangle){plr->position.x, plr->position.y - verticalStep, plrTextureRect.width, plrTextureRect.height};
		moved = true;

		if (!Assets->CheckColisions(Assets, futurePos)) {
			if (plr->position.y <= topLimit) {
				plr->position.y = topLimit;
			} else {
				plr->position.y -= verticalStep;
				plrCurrentFrame->y = 3;
			}
		}
	}

	if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
		futurePos = (Rectangle){plr->position.x - horizontalStep, plr->position.y, plrTextureRect.width, plrTextureRect.height};
		moved = true;

		if (!Assets->CheckColisions(Assets, futurePos)) {
			if (plr->position.x <= leftLimit) {
				plr->position.x = leftLimit;
			} else {
				plr->position.x -= horizontalStep;
				plrCurrentFrame->y = 2;
			}
		}
	}

	if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
		futurePos = (Rectangle){plr->position.x + horizontalStep, plr->position.y, plrTextureRect.width, plrTextureRect.height};
		moved = true;

		if (!Assets->CheckColisions(Assets, futurePos)) {
			if (plr->position.x >= rightLimit) {
				plr->position.x = rightLimit;
			} else {
				plr->position.x += horizontalStep;
				plrCurrentFrame->y = 4;
			}
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
	InitWindow(GetScreenWidth(), GetScreenHeight(), "ALTERNUDE");
	SearchAndSetResourceDir("resources");

	Spirit plrSpirit = {GetCenterOfScreen(), 8.5};
	Character* plr = (Character*) malloc(sizeof(Character));
	plr->hp = 100;
	plr->movespeed = 17.5;
	plr->spirit = plrSpirit;
	plr->position = GetCenterOfScreen();

	LoadedAssetsClass* Assets = InitLoadedAssets();
	Assets->plrTexture = LoadTexture("playr.png");

	bool spiritMode = false;
	bool isMoving = false;
	bool debugInfo = false;

	int lastScenario = -1;
	int scenario = 1;
	int plrFrameI = 0;
	Vector2 plrCurrentFrame = {0, 0};
	int plrFrameAmount = 4;
	int plrFrameSpeed = 3;
	Rectangle plrFrame = {0, 0, Assets->plrTexture.width / plrFrameAmount, Assets->plrTexture.height / 5};


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
		}
		if (debugInfo) { Assets->DrawHitboxes(Assets); }



		if (spiritMode) {
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

			DrawTextureRec(Assets->plrTexture, (Rectangle){plrFrame.width * plrCurrentFrame.x, plrFrame.height * plrCurrentFrame.y, plrFrame.width, plrFrame.height}, plr->position, WHITE);
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

	UnloadTexture(Assets->plrTexture);
	CloseWindow();

	Assets->FreeHitboxes(Assets);
	Assets->FreeImages(Assets);
	if (Assets != NULL) { free(Assets); }
	return 0;
}	
