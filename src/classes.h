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
	void (*NewHitbox)(struct LoadedAssetsClass* self, char*, Rectangle, bool, float);
	Hitbox* (*GetLastHitbox)(struct LoadedAssetsClass* self);
	void (*RemoveHitbox)(struct LoadedAssetsClass* self, Hitbox*);
	Hitbox* (*FindHitboxByName)(struct LoadedAssetsClass* self, char*);
	void (*FreeHitboxes)(struct LoadedAssetsClass* self);
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

#endif 