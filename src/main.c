#include <raylib.h>
#include <resource_dir.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

typedef struct LoadedTexture {
	char* name;
	Texture texture;
	struct LoadedTexture* next; 
} LoadedTexture;
typedef struct Hitbox {
	char* name;
	Rectangle rect;
	struct Hitbox* next;
} Hitbox;
typedef struct LoadedAssetsClass {
	Hitbox* hitboxHead;
	void (*NewHitbox)(struct LoadedAssetsClass*, char*, Rectangle);
	void (*PopHitbox)(struct LoadedAssetsClass*, Hitbox*);
	Hitbox* (*FindHitboxByName)(struct LoadedAssetsClass*, char*);
	void (*FreeHitboxes)(struct LoadedAssetsClass*);
	bool (*CheckColisions)(struct LoadedAssetsClass*, Rectangle);
	int (*GetHitboxAmount)(struct LoadedAssetsClass*);

	LoadedTexture* textureHead;
} LoadedAssetsClass;

typedef struct {
	LoadedImage* images;
	int imageCount;
	LoadedTexture* textures;
	int textureCount;
	Texture plrTexture;
} LoadedAssets;

void ____newhitboxfunction(LoadedAssetsClass* self, char* hitboxName, Rectangle hitboxRect) {
	Hitbox* node = (Hitbox*) malloc(sizeof(Hitbox));
	node->name = hitboxName;
	node->rect = hitboxRect;
	node->next = NULL;
	
	if (self->hitboxHead == NULL) {
		self->hitboxHead = node;
	} else {
		Hitbox* previousNode =self->hitboxHead;
		while (previousNode->next != NULL) { previousNode = previousNode->next; }
		previousNode->next = node;
	}	
}
void ____pophitboxfunction(LoadedAssetsClass* self, Hitbox* poppedNode){
	if (self->hitboxHead == NULL) {
		poppedNode = NULL;
	} else if (self->hitboxHead->next == NULL) {
		Hitbox* oldNode =self->hitboxHead;
		poppedNode = oldNode;
		self->hitboxHead = NULL;
	} else {
		Hitbox* previousNode =self->hitboxHead;
		while (previousNode->next->next != NULL) { previousNode = previousNode->next; }

		Hitbox* oldNode = previousNode->next;
		poppedNode = oldNode;
		previousNode->next = NULL;
	}
}
Hitbox* ____findhitboxfunction(LoadedAssetsClass* self, char* hitboxName) {
	if (self->hitboxHead == NULL) {
		return NULL;
	} else {
		Hitbox* previousNode =self->hitboxHead;
		Hitbox* result = NULL;

		while ((previousNode->next != NULL) || (strcmp(previousNode->name, hitboxName) != 0)) {
			previousNode = previousNode->next;
		}

		if (strcmp(previousNode->name, hitboxName) != 0) {
			return previousNode;
		} else {
			return NULL;
		}
	}
}
void ____freehitboxesfunction(LoadedAssetsClass* self) {
	if (self->hitboxHead != NULL) {
		free(self->hitboxHead);
	}
}
bool ____checkcolisionshitboxfunction(LoadedAssetsClass* self, Rectangle collider) {
	if (self->hitboxHead != NULL) {
		Hitbox* previousNode =self->hitboxHead;
		bool collided = CheckCollisionRecs(self->hitboxHead->rect, collider);

		while (previousNode->next != NULL && !collided) {
			previousNode = previousNode->next;
			if (CheckCollisionRecs(previousNode->rect, collider)) {
				collided = true;
				break;
			}
		}

		return collided;
	}

	return false;
}
int ____hitboxamountfunction(LoadedAssetsClass* self) {
	int amt = 0;

	if (self->hitboxHead != NULL) {
		Hitbox* previousNode = self->hitboxHead;
		amt++;

		while (previousNode->next != NULL) {
			amt++;
		}
	}

	return amt;
}

LoadedAssetsClass* InitLoadedAssets() {
	LoadedAssetsClass* assets = (LoadedAssetsClass*) malloc(sizeof(LoadedAssetsClass));

	assets->hitboxHead = NULL;
	assets->NewHitbox = ____newhitboxfunction;
	assets->PopHitbox = ____pophitboxfunction;
	assets->FindHitboxByName = ____findhitboxfunction;
	assets->FreeHitboxes = ____freehitboxesfunction;
	assets->CheckColisions = ____checkcolisionshitboxfunction;
	assets->GetHitboxAmount = ____hitboxamountfunction;

	assets->textureHead = NULL;

	return assets;
}



bool DEBUG_MODE = true;
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

bool MovePlr(Character* plr, Rectangle plrTextureRect, Vector2* plrCurrentFrame, LoadedAssetsClass* Assets) {
	float speed       = plr->movespeed;
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
		Rectangle futurePos = {plr->position.x, plr->position.y + verticalStep, plrTextureRect.width, plrTextureRect.height};
		
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
		Rectangle futurePos = {plr->position.x, plr->position.y - verticalStep, plrTextureRect.width, plrTextureRect.height};

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
		Rectangle futurePos = {plr->position.x - horizontalStep, plr->position.y, plrTextureRect.width, plrTextureRect.height};

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
		Rectangle futurePos = {plr->position.x + horizontalStep, plr->position.y, plrTextureRect.width, plrTextureRect.height};

		if (!Assets->CheckColisions(Assets, futurePos)) {
			if (plr->position.x >= rightLimit) {
				plr->position.x = rightLimit;
			} else {
				plr->position.x += horizontalStep;
				plrCurrentFrame->y = 4;
			}
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


bool DrawGameHitboxes(LoadedAssetsClass* self) {
	Color hitboxColor = BLUE;

	if (self->hitboxHead != NULL) {
		Hitbox* previousNode =self->hitboxHead;
		DrawRectangleRec(previousNode->rect, hitboxColor);

		while (previousNode->next != NULL) {
			previousNode = previousNode->next;
			DrawRectangleRec(previousNode->rect, hitboxColor);
		}
	}
}

void DrawGameScenarios(int scenarioIndex, LoadedAssets* assets, LoadedAssetsClass* Assets) {
	Color hitboxColor = DEBUG_MODE ? BLUE : (Color){0, 0, 0, 1};

	//if (lastScenario != scenarioIndex) {
		//lastScenario = scenarioIndex;

		for (int i = 0; i < assets->imageCount; i++) {
		UnloadImage(assets->images[i].image);
		}

		for (int i = 0; i < assets->textureCount; i++) {
			UnloadTexture(assets->textures[i].texture);
		}

		if (assets->images != NULL) { free(assets->images); }
		if (assets->textures != NULL) { free(assets->textures); }
		Assets->FreeHitboxes(Assets);
	//}
	
	

	if (scenarioIndex == 0) { // Kard Kassino Garden
		Rectangle hb1 = {0, GetScreenHeight() - 350, 300, 300};
		Assets->NewHitbox(Assets, "KardKassinoGarden_HB1", hb1);
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
	Character plr = {GetCenterOfScreen(), 17.7, 100, plrSpirit};

	LoadedAssetsClass* Assets = InitLoadedAssets();

	LoadedAssets *assets = malloc(sizeof(LoadedAssets));
	assets->images = NULL;
	assets->textures = NULL;
	assets->imageCount = 0;
	assets->textureCount = 0;
	assets->plrTexture = LoadTexture("playr.png");

	bool spiritMode = false;
	bool isMoving = false;
	bool debugInfo = false;

	int lastScenario = -1;
	int scenario = 0;
	int plrFrameI = 0;
	Vector2 plrCurrentFrame = {0, 0};
	int plrFrameAmount = 4;
	int plrFrameSpeed = 3;
	Rectangle plrFrame = {0, 0, assets->plrTexture.width / plrFrameAmount, assets->plrTexture.height / 5};


	while (!WindowShouldClose()) {
		if (IsKeyPressed(KEY_P)) {
			debugInfo = !debugInfo;
		}

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
			DrawGameScenarios(scenario, assets, Assets);
		}
		if (debugInfo) { DrawGameHitboxes(Assets); }



		if (spiritMode) {
			MoveSpirit(&plrSpirit);

			DrawBattleBackground(getBattleSquareSize());
			DrawRectangle(plrSpirit.position.x - (playerSpiritSize / 2), plrSpirit.position.y - (playerSpiritSize / 2), playerSpiritSize, playerSpiritSize, RED);
			if (debugInfo) { DrawText("Spirit Mode: ON   ([SPACE] to toggle!)", 15, 50, 20, LIGHTGRAY); }
		} else {
			bool hasMovement = MovePlr(&plr, plrFrame, &plrCurrentFrame, Assets);

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
			if (debugInfo) { DrawText("Spirit Mode: OFF ([SPACE] to toggle!)", 15, 50, 20, LIGHTGRAY); }
			if (debugInfo) { DrawText(TextFormat("Player Sprite State - (X:%.0f, Y:%.0f)", plrCurrentFrame.x, plrCurrentFrame.y), 15, GetScreenHeight() - 40, 20, LIGHTGRAY); }
		}
		
		DrawText("Press [P] to show/hide debug info!", 15, 20, 30, WHITE);
		if (debugInfo) { DrawText(TextFormat("HP: %d", plr.hp), 15, 70, 20, RED); }
		if (debugInfo) { DrawText(TextFormat("Loaded Hitboxes: %d", Assets->GetHitboxAmount(Assets)), 15, 90, 20, BLUE); }
		EndDrawing();
	}

	UnloadTexture(assets->plrTexture);
	CloseWindow();

	Assets->FreeHitboxes(Assets);
	if (Assets != NULL) { free(Assets); }

	if (assets->images != NULL) { free(assets->images); }
	if (assets != NULL) { free(assets); }
	return 0;
}	
