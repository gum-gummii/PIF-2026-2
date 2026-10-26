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

	LoadedImage* imageHead;
	void (*NewImage)(struct LoadedAssetsClass*, char*, Image);
	LoadedImage* (*GetLastImage)(struct LoadedAssetsClass*);
	void (*RemoveImage)(struct LoadedAssetsClass*, LoadedImage*);
	LoadedImage* (*FindImageByName)(struct LoadedAssetsClass*, char*);
	void (*FreeImages)(struct LoadedAssetsClass*);
	int (*GetImageAmount)(struct LoadedAssetsClass*);
	void (*UnloadImages)(struct LoadedAssetsClass*);

	Texture plrTexture;
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
void ____freehitboxesfunction(LoadedAssetsClass* self) {
	if (self->hitboxHead != NULL) {
		free(self->hitboxHead);
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
	assets->FreeHitboxes = ____freehitboxesfunction;
	assets->CheckColisions = __checkHitboxColision;
	assets->CheckDamageColisions = __checkDamageColision;
	assets->GetHitboxAmount = ____hitboxamountfunction;
	assets->DrawHitboxes = __drawHitbox;

	assets->textureHead = NULL;

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