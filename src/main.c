#include <raylib.h>
#include <resource_dir.h>
#include <stdio.h>
#include <stdlib.h>
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

typedef struct {
	char* name;
	Image image;
} LoadedImage;

typedef struct {
	char* name;
	Texture texture;
} LoadedTexture;

typedef struct {
	char* name;
	Rectangle rect;
} Hitbox;

typedef struct {
	LoadedImage* images;
	int imageCount;
	LoadedTexture* textures;
	int textureCount;
	Hitbox* hitboxes;
	int hitboxCount;

	Texture plrTexture;
} LoadedAssets;


bool DEBUG_MODE = true;
int playerSpiritSize = 40;
int getBattleSquareSize() {
	return GetScreenHeight() - 50;
}
int getBattleBorderSize() {
	return (int)round(getBattleSquareSize() / 50);
}


float GetFrameScale() {
	return fminf(GetFrameTime(), 0.05f) * 60.0f;
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

	DrawRectangleRec(box, bgColor);
	DrawRectangleLinesEx(box, getBattleBorderSize(), GREEN);
}



void MoveSpirit(Spirit* plrSpirit) {
	Vector2 center    = GetCenterOfScreen();
	float speed       = plrSpirit->movespeed * GetFrameScale();
	float sqrtSpeed   = speed / sqrt(2);

	float leftLimit   = center.x - (getBattleSquareSize() / 2) + (playerSpiritSize / 2) + getBattleBorderSize();
	float topLimit    = center.y - (getBattleSquareSize() / 2) + (playerSpiritSize / 2) + getBattleBorderSize();
	float rightLimit  = center.x - (playerSpiritSize / 2) + (getBattleSquareSize() / 2) - getBattleBorderSize();
	float bottomLimit = center.y - (playerSpiritSize / 2) + (getBattleSquareSize() / 2) - getBattleBorderSize();

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

bool CheckColisions(Hitbox* hitboxes, int hitboxCount, Rectangle plrRect) {
	for (int i = 0; i < hitboxCount; i++) {
		if (CheckCollisionRecs(hitboxes[i].rect, plrRect)) {
			return true;
		}
	}
	return false;
}

bool MovePlr(Character* plr, Rectangle plrTextureRect, Vector2* plrCurrentFrame, LoadedAssets* assets) {
	float speed       = plr->movespeed * GetFrameScale();
	float sqrtSpeed   = speed / sqrt(2);

	float leftLimit   = 0;
	float topLimit    = 0;
	float rightLimit  = GetScreenWidth() - plrTextureRect.width;
	float bottomLimit = GetScreenHeight() - plrTextureRect.height;

	bool hasVerticalMovement   = IsKeyDown(KEY_UP)   || IsKeyDown(KEY_W) || IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S);
	bool hasHorizontalMovement = IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A) || IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);

	float verticalStep = hasHorizontalMovement ? sqrtSpeed : speed;
	float horizontalStep = hasVerticalMovement ? sqrtSpeed : speed;


	if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
		float newY = fminf(plr->position.y + verticalStep, bottomLimit);
		Rectangle futurePos = {plr->position.x, newY, plrTextureRect.width, plrTextureRect.height};

		if (!CheckColisions(assets->hitboxes, assets->hitboxCount, futurePos)) {
			plr->position.y = newY;
			plrCurrentFrame->y = 1;
		}
	}

	if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
		float newY = fmaxf(plr->position.y - verticalStep, topLimit);
		Rectangle futurePos = {plr->position.x, newY, plrTextureRect.width, plrTextureRect.height};

		if (!CheckColisions(assets->hitboxes, assets->hitboxCount, futurePos)) {
			plr->position.y = newY;
			plrCurrentFrame->y = 3;
		}
	}

	if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
		float newX = fmaxf(plr->position.x - horizontalStep, leftLimit);
		Rectangle futurePos = {newX, plr->position.y, plrTextureRect.width, plrTextureRect.height};

		if (!CheckColisions(assets->hitboxes, assets->hitboxCount, futurePos)) {
			plr->position.x = newX;
			plrCurrentFrame->y = 2;
		}
	}

	if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
		float newX = fminf(plr->position.x + horizontalStep, rightLimit);
		Rectangle futurePos = {newX, plr->position.y, plrTextureRect.width, plrTextureRect.height};

		if (!CheckColisions(assets->hitboxes, assets->hitboxCount, futurePos)) {
			plr->position.x = newX;
			plrCurrentFrame->y = 4;
		}
	}

	return hasHorizontalMovement || hasVerticalMovement;
}

void UnloadScenario(LoadedAssets* assets) {
	for (int i = 0; i < assets->imageCount; i++) {
		UnloadImage(assets->images[i].image);
	}

	for (int i = 0; i < assets->textureCount; i++) {
		UnloadTexture(assets->textures[i].texture);
	}

	if (assets->images != NULL) { free(assets->images); }
	if (assets->textures != NULL) { free(assets->textures); }
	if (assets->hitboxes != NULL) { free(assets->hitboxes); }

	assets->images = NULL;
	assets->textures = NULL;
	assets->hitboxes = NULL;
	assets->imageCount = 0;
	assets->textureCount = 0;
	assets->hitboxCount = 0;
}

void LoadScenario(int scenarioIndex, LoadedAssets* assets) {
	UnloadScenario(assets);

	if (scenarioIndex == 0) { // Kard Kassino Garden
		assets->hitboxes = (Hitbox*) malloc(sizeof(Hitbox) * 1);

		Rectangle hb1 = {0, GetScreenHeight() - 350, 300, 300};
		assets->hitboxes[0] = (Hitbox){"Kard Kassino Garden Hitbox 1", hb1};

		assets->hitboxCount = 1;
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

void DrawScenario(LoadedAssets* assets) {
	if (!DEBUG_MODE) return;

	for (int i = 0; i < assets->hitboxCount; i++) {
		DrawRectangleRec(assets->hitboxes[i].rect, BLUE);
	}
}



int main() {
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(1280, 720, "ALTERNUDE");
	SearchAndSetResourceDir("resources");

	Spirit plrSpirit = {GetCenterOfScreen(), 8.5};
	Character plr = {GetCenterOfScreen(), 17.7, 100, plrSpirit};

	LoadedAssets *assets = malloc(sizeof(LoadedAssets));
	assets->images = NULL;
	assets->textures = NULL;
	assets->hitboxes = NULL;
	assets->imageCount = 0;
	assets->textureCount = 0;
	assets->hitboxCount = 0;

	assets->plrTexture = LoadTexture("playr.png");
	if (assets->plrTexture.id == 0) {
		TraceLog(LOG_ERROR, "Failed to load playr.png (is the resources folder reachable?)");
		free(assets);
		CloseWindow();
		return 1;
	}

	bool spiritMode = false;
	bool isMoving = false;

	int scenario = 0;
	LoadScenario(scenario, assets);
	int plrFrameI = 0;
	Vector2 plrCurrentFrame = {0, 0};
	int plrFrameAmount = 4;
	int plrFrameSpeed = 3;
	Rectangle plrFrame = {0, 0, assets->plrTexture.width / plrFrameAmount, assets->plrTexture.height / 5};


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
		DrawScenario(assets);



		if (spiritMode) {
			MoveSpirit(&plrSpirit);

			DrawBattleBackground(getBattleSquareSize());
			DrawRectangle(plrSpirit.position.x - (playerSpiritSize / 2), plrSpirit.position.y - (playerSpiritSize / 2), playerSpiritSize, playerSpiritSize, RED);
			DrawText("Spirit Mode: TRUE", 20, 50, 30, LIGHTGRAY);
		} else {
			bool hasMovement = MovePlr(&plr, plrFrame, &plrCurrentFrame, assets);

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

			DrawTextureRec(assets->plrTexture, (Rectangle){plrFrame.width * plrCurrentFrame.x, plrFrame.height * plrCurrentFrame.y, plrFrame.width, plrFrame.height}, plr.position, WHITE);
			DrawText("Spirit Mode: FALSE", 20, 50, 30, LIGHTGRAY);
			DrawText(TextFormat("Player Sprite State - (X:%.0f, Y:%.0f)", plrCurrentFrame.x, plrCurrentFrame.y), 20, GetScreenHeight() - 50, 30, LIGHTGRAY);
		}
		
		DrawText("[SPACE] to toggle spirit mode!", 20, 20, 30, WHITE);
		EndDrawing();
	}

	UnloadScenario(assets);
	UnloadTexture(assets->plrTexture);
	CloseWindow();

	free(assets);
	return 0;
}	
