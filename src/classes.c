#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct LoadedImage {
	char* name;
	Image image;
	struct LoadedImage* next;
} LoadedImage;
typedef struct LoadedTexture {
	char* name;
	Texture texture;
	struct LoadedTexture* next; 
} LoadedTexture;
typedef struct Hitbox {
	char* name;
	Rectangle rect;
	bool canCollide;
	float dmg;
	struct Hitbox* next;
} Hitbox;
typedef struct LoadedAssetsClass {
	Hitbox* hitboxHead;
	void (*NewHitbox)(struct LoadedAssetsClass*, char*, Rectangle, bool, float);
	Hitbox* (*GetLastHitbox)(struct LoadedAssetsClass*);
	void (*RemoveHitbox)(struct LoadedAssetsClass*, Hitbox*);
	Hitbox* (*FindHitboxByName)(struct LoadedAssetsClass*, char*);
	void (*FreeHitboxes)(struct LoadedAssetsClass*);
	bool (*CheckColisions)(struct LoadedAssetsClass*, Rectangle);
	float (*CheckDamageColisions)(struct LoadedAssetsClass*, Rectangle);
	int (*GetHitboxAmount)(struct LoadedAssetsClass*);
	void (*DrawHitboxes)(struct LoadedAssetsClass*);

	LoadedTexture* textureHead;
	void (*NewTexture)(struct LoadedAssetsClass*, char*, Texture);
	LoadedTexture* (*GetLastTexture)(struct LoadedAssetsClass*);
	void (*RemoveTexture)(struct LoadedAssetsClass*, LoadedTexture*);
	LoadedTexture* (*FindTextureByName)(struct LoadedAssetsClass*, char*);
	void (*FreeTextures)(struct LoadedAssetsClass*);
	int (*GetTextureAmount)(struct LoadedAssetsClass*);

	LoadedImage* imageHead;
	void (*NewImage)(struct LoadedAssetsClass*, char*, Image);
	LoadedImage* (*GetLastImage)(struct LoadedAssetsClass*);
	void (*RemoveImage)(struct LoadedAssetsClass*, LoadedImage*);
	LoadedImage* (*FindImageByName)(struct LoadedAssetsClass*, char*);
	void (*FreeImages)(struct LoadedAssetsClass*);
	int (*GetImageAmount)(struct LoadedAssetsClass*);
	void (*UnloadImages)(struct LoadedAssetsClass*);
} LoadedAssetsClass;


void __newHitbox(LoadedAssetsClass* self, char* hitboxName, Rectangle hitboxRect, bool canCollide, float dmg) {
	Hitbox* node = (Hitbox*) malloc(sizeof(Hitbox));
	node->name = hitboxName;
	node->rect = hitboxRect;
	node->dmg = dmg;
	node->canCollide = canCollide;
	node->next = NULL;
	
	if (self->hitboxHead == NULL) {
		self->hitboxHead = node;
	} else {
		Hitbox* previousNode =self->hitboxHead;
		while (previousNode->next != NULL) { previousNode = previousNode->next; }
		previousNode->next = node;
	}	
}
Hitbox* ____getlasthitboxfunction(LoadedAssetsClass* self) {
	if (self->hitboxHead != NULL) {
		Hitbox* lastNode = self->hitboxHead;

		while (lastNode->next != NULL) {
			lastNode = lastNode->next;
		}

		return lastNode;
	}

	return NULL;
}
void ____removehitboxbyptrfunction(LoadedAssetsClass* self, Hitbox* ptrHitbox) {
	if (self->hitboxHead == ptrHitbox) {
		self->hitboxHead = (self->hitboxHead->next != NULL) ? self->hitboxHead->next : NULL;
		free(ptrHitbox);
	} else {
		Hitbox* previousNode = self->hitboxHead;
		while (previousNode->next != ptrHitbox) {
			previousNode = previousNode->next;
		}

		previousNode->next = (previousNode->next->next != NULL) ? previousNode->next->next : NULL;
		free(ptrHitbox);
	}
}
Hitbox* ____findhitboxfunction(LoadedAssetsClass* self, char* hitboxName) {
	if (self->hitboxHead != NULL) {
		Hitbox* node = self->hitboxHead;
		Hitbox* result = NULL;

		while ((node->next != NULL) || (strcmp(node->name, hitboxName) != 0)) {
			node = node->next;
		}

		if (strcmp(node->name, hitboxName) != 0) {
			return node;
		}
	}

	return NULL;
}
void __freeHitboxes(LoadedAssetsClass* self) {
	if (self->hitboxHead != NULL) {
		free(self->hitboxHead);
		self->hitboxHead = NULL;
	}
}
bool __checkHitboxColision(LoadedAssetsClass* self, Rectangle collider) {
	if (self->hitboxHead != NULL) {
		Hitbox* previousNode =self->hitboxHead;
		bool collided = CheckCollisionRecs(self->hitboxHead->rect, collider) && self->hitboxHead->canCollide;

		while (previousNode->next != NULL && !collided) {
			previousNode = previousNode->next;
			if (CheckCollisionRecs(previousNode->rect, collider) && previousNode->canCollide) {
				collided = true;
				break;
			}
		}

		return collided;
	}

	return false;
}
float __checkDamageColision(LoadedAssetsClass* self, Rectangle collider) {
	if (self->hitboxHead != NULL) {
		Hitbox* previousNode =self->hitboxHead;
		
		if (CheckCollisionRecs(self->hitboxHead->rect, collider) && self->hitboxHead->dmg != 0) {
			return self->hitboxHead->dmg;
		}

		while (previousNode->next != NULL) {
			previousNode = previousNode->next;
			if (CheckCollisionRecs(previousNode->rect, collider) && previousNode->dmg != 0) {
				return previousNode->dmg;
			}
		}
	}

	return 0.0;
}
int ____hitboxamountfunction(LoadedAssetsClass* self) {
	int amt = 0;

	if (self->hitboxHead != NULL) {
		Hitbox* previousNode = self->hitboxHead;
		amt++;

		while (previousNode->next != NULL) {
			amt++;
			previousNode = previousNode->next;
		}
	}

	return amt;
}
Color ____gethitboxcolor(float dmg) {
	if (dmg > 0.01) {
		return GRAY;
	} else if (dmg < -0.01) {
		return LIME;
	} else {
		return BLUE;
	}
}
void __drawHitbox(LoadedAssetsClass* self) {
	if (self->hitboxHead != NULL) {
		Hitbox* previousNode =self->hitboxHead;

		DrawRectangleRec(previousNode->rect, ____gethitboxcolor(previousNode->dmg));

		while (previousNode->next != NULL) {
			previousNode = previousNode->next;
			DrawRectangleRec(previousNode->rect, ____gethitboxcolor(previousNode->dmg));
		}
	}
}


void __newTexture(LoadedAssetsClass* self, char* textureName, Texture texture) {
	if (texture.id == 0) {
		TraceLog(LOG_ERROR, TextFormat("Failed to load %s (is the resources folder reachable?)", textureName));
		return;
	}

	LoadedTexture* node = (LoadedTexture*) malloc(sizeof(LoadedTexture));
	node->name = textureName;
	node->texture = texture;
	node->next = NULL;
	
	if (self->textureHead == NULL) {
		self->textureHead = node;
	} else {
		LoadedTexture* previousNode =self->textureHead;
		while (previousNode->next != NULL) { previousNode = previousNode->next; }
		previousNode->next = node;
	}	
}
LoadedTexture* __getLastTexture(LoadedAssetsClass* self) {
	if (self->textureHead != NULL) {
		LoadedTexture* lastNode = self->textureHead;

		while (lastNode->next != NULL) {
			lastNode = lastNode->next;
		}

		return lastNode;
	}

	return NULL;
}
void __removeTextureByPtr(LoadedAssetsClass* self, LoadedTexture* ptrTexture) {
	if (ptrTexture == NULL) { return; }

	if (self->textureHead == ptrTexture) {
		self->textureHead = (self->textureHead->next != NULL) ? self->textureHead->next : NULL;
		free(ptrTexture);
	} else {
		LoadedTexture* previousNode = self->textureHead;
		while (previousNode->next != ptrTexture) {
			previousNode = previousNode->next;
		}

		previousNode->next = (previousNode->next->next != NULL) ? previousNode->next->next : NULL;
		free(ptrTexture);
	}
}
LoadedTexture* __findTexture(LoadedAssetsClass* self, char* textureName) {
	if (self->textureHead != NULL) {
		LoadedTexture* node = self->textureHead;

		while ((node->next != NULL) || (strcmp(node->name, textureName) != 0)) {
			node = node->next;
		}

		if (strcmp(node->name, textureName) != 0) {
			return node;
		} else {
			return NULL;
		}
	}

	return NULL;
}
void __freeTextures(LoadedAssetsClass* self) {
	if (self->textureHead != NULL) {
		free(self->textureHead);
		self->textureHead = NULL;
	}
}
int __getTextureAmount(LoadedAssetsClass* self) {
	int amt = 0;

	if (self->textureHead != NULL) {
		LoadedTexture* previousNode = self->textureHead;
		amt++;

		while (previousNode->next != NULL) {
			amt++;
			previousNode = previousNode->next;
		}
	}

	return amt;
}


void __newImage(LoadedAssetsClass* self, char* imageName, Image image) {
	LoadedImage* node = (LoadedImage*) malloc(sizeof(LoadedImage));
	node->name = imageName;
	node->image = image;
	node->next = NULL;
	
	if (self->imageHead == NULL) {
		self->imageHead = node;
	} else {
		LoadedImage* previousNode =self->imageHead;
		while (previousNode->next != NULL) { previousNode = previousNode->next; }
		previousNode->next = node;
	}	
}
LoadedImage* __getLastImage(LoadedAssetsClass* self) {
	if (self->imageHead != NULL) {
		LoadedImage* lastNode = self->imageHead;

		while (lastNode->next != NULL) {
			lastNode = lastNode->next;
		}

		return lastNode;
	}

	return NULL;
}
void __removeImageByPtr(LoadedAssetsClass* self, LoadedImage* ptrImage) {
	if (self->imageHead == ptrImage) {
		self->imageHead = (self->imageHead->next != NULL) ? self->imageHead->next : NULL;
		free(ptrImage);
	} else {
		LoadedImage* previousNode = self->imageHead;
		while (previousNode->next != ptrImage) {
			previousNode = previousNode->next;
		}

		previousNode->next = (previousNode->next->next != NULL) ? previousNode->next->next : NULL;
		free(ptrImage);
	}
}
LoadedImage* __findImage(LoadedAssetsClass* self, char* imageName) {
	if (self->imageHead != NULL) {
		LoadedImage* node = self->imageHead;
		LoadedImage* result = NULL;

		while ((node->next != NULL) || (strcmp(node->name, imageName) != 0)) {
			node = node->next;
		}

		if (strcmp(node->name, imageName) != 0) {
			return node;
		}
	}

	return NULL;
}
void __freeImages(LoadedAssetsClass* self) {
	self->UnloadImages(self);

	if (self->imageHead != NULL) {
		free(self->imageHead);
		self->imageHead = NULL;
	}
}
int __imageAmount(LoadedAssetsClass* self) {
	int amt = 0;

	if (self->imageHead != NULL) {
		LoadedImage* previousNode = self->imageHead;
		amt++;

		while (previousNode->next != NULL) {
			amt++;
			previousNode = previousNode->next;
		}
	}

	return amt;
}
void __unloadImages(LoadedAssetsClass* self) {
	if (self->imageHead != NULL) {
		LoadedImage* previousNode = self->imageHead;
		UnloadImage(previousNode->image);

		while (previousNode->next != NULL) {
			previousNode = previousNode->next;
			UnloadImage(previousNode->image);
		}
	}
}


LoadedAssetsClass* InitLoadedAssets() {
	LoadedAssetsClass* assets = (LoadedAssetsClass*) malloc(sizeof(LoadedAssetsClass));

	assets->hitboxHead = NULL;
	assets->NewHitbox = __newHitbox;
	assets->GetLastHitbox = ____getlasthitboxfunction;
	assets->RemoveHitbox = ____removehitboxbyptrfunction;
	assets->FindHitboxByName = ____findhitboxfunction;
	assets->FreeHitboxes = __freeHitboxes;
	assets->CheckColisions = __checkHitboxColision;
	assets->CheckDamageColisions = __checkDamageColision;
	assets->GetHitboxAmount = ____hitboxamountfunction;
	assets->DrawHitboxes = __drawHitbox;

	assets->textureHead = NULL;
	assets->NewTexture = __newTexture;
	assets->GetLastTexture = __getLastTexture;
	assets->RemoveTexture = __removeTextureByPtr;
	assets->FindTextureByName = __findTexture;
	assets->FreeTextures = __freeTextures;
	assets->GetTextureAmount = __getTextureAmount;

	assets->imageHead = NULL;
	assets->NewImage = __newImage;
	assets->GetLastImage = __getLastImage;
	assets->RemoveImage = __removeImageByPtr;
	assets->FindImageByName = __findImage;
	assets->FreeImages = __freeImages;
	assets->GetImageAmount = __imageAmount;
	assets->UnloadImages = __unloadImages;

	return assets;
}









typedef struct LineBulletNode {
	struct LineBulletNode* next;
	Vector2 positionStart;
	Vector2 positionEnd;
	Color color;
} LineBulletNode;
typedef struct BulletClass {
	LineBulletNode* lineBulletsHead;
	LineBulletNode* (*NewLineBullet)(struct BulletClass* self, Color color, Vector2 posStart, Vector2 posEnd);
} BulletClass;

LineBulletNode* __newLineBullet(BulletClass* self, Color color, Vector2 posStart, Vector2 posEnd) {
	LineBulletNode* node = (LineBulletNode*) malloc(sizeof(LineBulletNode));
	node->next = NULL;
	node->color = color;
	node->positionEnd = posEnd;
	node->positionStart = posStart;
	
	if (self->lineBulletsHead == NULL) {
		self->lineBulletsHead = node;
	} else {
		LineBulletNode* previousNode =self->lineBulletsHead;
		while (previousNode->next != NULL) { previousNode = previousNode->next; }
		previousNode->next = node;
	}

	return node;
}
void __freeBullets(BulletClass* self) {
	if (self->lineBulletsHead != NULL) {
		free(self->lineBulletsHead);
		self->lineBulletsHead = NULL;
	}
}
void __removeLineBulletByPtr(BulletClass* self, LineBulletNode* ptrBullet) {
	if (self->lineBulletsHead == ptrBullet) {
		self->lineBulletsHead = (self->lineBulletsHead->next != NULL) ? self->lineBulletsHead->next : NULL;
		free(ptrBullet);
	} else {
		LineBulletNode* previousNode = self->lineBulletsHead;
		while (previousNode->next != ptrBullet) {
			previousNode = previousNode->next;
		}

		previousNode->next = (previousNode->next->next != NULL) ? previousNode->next->next : NULL;
		free(ptrBullet);
	}
}









typedef struct {
	Vector2 position;
	float movespeed;
} Spirit;

typedef struct {
	Vector2 position;
	Texture texture;
	float movespeed;
	int hp;
	int maxHp;
	Rectangle animFrame;
	int animTick;
} Character;

typedef struct GameDataClass{
	Character* mainCharacter;
	Character* sideCharacter_1;
	Character* sideCharacter_2;
	Spirit* spirit;

	bool debugMode;
	bool spiritMode;

	Character* (*NewCharacter)(struct GameDataClass* self, Vector2 pos, float speed, int hp, Texture texture, Vector2 animFrameDivisions);
	void (*UnloadCharacter)(struct GameDataClass* self, Character* character);
} GameDataClass;



Character* __newCharacter(GameDataClass* self, Vector2 pos, float speed, int hp, Texture texture, Vector2 animFrameDivisions) {
	if (texture.id == 0) {
		TraceLog(LOG_ERROR, "Failed to load character texture (is the resources folder reachable?)");
		free(self);
		CloseWindow();
		return NULL;
	}

	Character* character = (Character*) malloc(sizeof(Character));
	character->position = pos;
	character->movespeed = speed;
	character->hp = hp;
	character->maxHp = hp;
	character->texture = texture;
	character->animFrame = (Rectangle){0, 0, character->texture.width / animFrameDivisions.x, character->texture.height / animFrameDivisions.y};

	return character;
}
void __unloadCharacter(GameDataClass* self, Character* character) {
	if (character != NULL) {
		UnloadTexture(character->texture);
		free(character);
	}
}



GameDataClass* InitGameData() {
	GameDataClass* GameData = (GameDataClass*) malloc(sizeof(GameDataClass));

	GameData->mainCharacter = NULL;
	GameData->sideCharacter_1 = NULL;
	GameData->sideCharacter_2 = NULL;

	GameData->spirit = (Spirit*) malloc(sizeof(Spirit));
	if (GameData->spirit == NULL) {
		free(GameData);
		return NULL;
	}
	GameData->spirit->position = (Vector2){GetScreenWidth()/2, GetScreenHeight()/2};
	GameData->spirit->movespeed = 8.5;

	GameData->debugMode = false;
	GameData->spiritMode = false;


	GameData->NewCharacter = __newCharacter;
	GameData->UnloadCharacter = __unloadCharacter;

	return GameData;
}
