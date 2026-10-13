#include <raylib.h>
#include <resource_dir.h>
#include <stdio.h>
#include <math.h>


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


bool DEBUG_MODE = false;
int playerSpiritSize = 40;
int getBattleSquareSize() {
	return GetScreenHeight() - 50;
}
int getBattleBorderSize() {
	return (int)round(getBattleSquareSize() / 50);
}


Vector2 GetCenterOfScreen() {
	float x = GetScreenWidth() / 2;
	float y = GetScreenHeight() / 2;
	
	return (Vector2){x, y};
}

void DrawBattleBackground(int squareSize) {
	float x = (GetScreenWidth() / 2) - (squareSize / 2);
	float y = (GetScreenHeight() / 2) - (squareSize / 2);

	Rectangle box = {x, y, squareSize, squareSize};
	Color bgColor = {0, 0, 0, 100};

	DrawRectangle(x, y, squareSize, squareSize, bgColor);
	DrawRectangleRec(box, bgColor);
	DrawRectangleLinesEx(box, getBattleBorderSize(), GREEN);
}



void MoveSpirit(Spirit* plrSpirit) {
	Vector2 center    = GetCenterOfScreen();
	float speed       = plrSpirit->movespeed;
	float sqrtSpeed   = speed / sqrt(2);

	float leftLimit   = center.x - (getBattleSquareSize() / 2) + (playerSpiritSize / 2) + getBattleBorderSize();
	float topLimit    = center.y - (getBattleSquareSize() / 2) + (playerSpiritSize / 2) + getBattleBorderSize();
	float rightLimit  = center.x - (playerSpiritSize / 2) + (getBattleSquareSize() / 2) - getBattleBorderSize();
	float bottomLimit = center.y - (playerSpiritSize / 2) + (getBattleSquareSize() / 2) - getBattleBorderSize();


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

void MovePlr(Character* plr, Texture plrTexture) {
	float speed       = plr->movespeed;
	float sqrtSpeed   = speed / sqrt(2);

	float leftLimit   = 0;
	float topLimit    = 0;
	float rightLimit  = GetScreenWidth() - plrTexture.width * 2;
	float bottomLimit = GetScreenHeight() - plrTexture.height * 2;

	bool hasVerticalMovement   = IsKeyDown(KEY_UP)   || IsKeyDown(KEY_W) || IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S);
	bool hasHorizontalMovement = IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A) || IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);


	if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
		if (plr->position.x > leftLimit) {
			plr->position.x -= hasVerticalMovement ? sqrtSpeed : speed;
		} else {
			plr->position.x = leftLimit;
		}
	}

	if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
		if (plr->position.x < rightLimit) {
			plr->position.x += hasVerticalMovement ? sqrtSpeed : speed;
		} else {
			plr->position.x = rightLimit;
		}
	}

	if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
		if (plr->position.y > topLimit) {
			plr->position.y -= hasHorizontalMovement ? sqrtSpeed : speed;
		} else {
			plr->position.y = topLimit;
		}
	}

	if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
		if (plr->position.y < bottomLimit) {
			plr->position.y += hasHorizontalMovement ? sqrtSpeed : speed;
		} else {
			plr->position.y = bottomLimit;
		}
	}
}


// HARD CODED SCENARIOS
void DrawGameScenarios(int scenarioIndex) {
	Color hitboxColor = DEBUG_MODE ? BLUE : (Color){0, 0, 0, 1};

	if (scenarioIndex == 0) { // Kard Kassino Garden
		

	} else if (scenarioIndex == 1) { // Kard Kassino Entrace

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
	Character plr = {GetCenterOfScreen(), 13.7, 100, plrSpirit};
	Texture wabbit = LoadTexture("wabbit_alpha.png");

	bool spiritMode = false;
	int scenario = 0;


	while (!WindowShouldClose()) {
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
		//DrawGameScenarios(scenario);



		if (spiritMode) {
			MoveSpirit(&plrSpirit);

			DrawBattleBackground(getBattleSquareSize());
			DrawRectangle(plrSpirit.position.x - (playerSpiritSize / 2), plrSpirit.position.y - (playerSpiritSize / 2), playerSpiritSize, playerSpiritSize, RED);
			DrawText("Spirit Mode: TRUE", 20, 50, 30, LIGHTGRAY);
		} else {
			MovePlr(&plr, wabbit);

			DrawTextureEx(wabbit, plr.position, 0, 2, PINK);
			DrawText("Spirit Mode: FALSE", 20, 50, 30, LIGHTGRAY);
		}
		
		DrawText("[SPACE] to toggle spirit mode!", 20, 20, 30, WHITE);
		
		
		
		EndDrawing();
	}

	UnloadTexture(wabbit);
	CloseWindow();
	return 0;
}
