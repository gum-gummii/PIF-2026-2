#ifndef CLASSES_H
#define CLASSES_H

#include <raylib.h>



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

LoadedAssetsClass* InitLoadedAssets();

#endif 