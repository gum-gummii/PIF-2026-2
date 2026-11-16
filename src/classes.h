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
GameDataClass* InitGameData();

#endif