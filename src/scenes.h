#ifndef SCENES_H
#define SCENES_H

#include <raylib.h>
#include <classes.h>
#include <door.h>

#define MAX_SCENE_EXITS 3

typedef enum {
	SCENE_GARDEN,
	SCENE_PARKING,
	SCENE_ENTRANCE,
	SCENE_ROOM1,
	SCENE_ROOM2,
	SCENE_KEY_ROOM,
	SCENE_BOSS_ROOM,
	SCENE_COUNT
} SceneId;

typedef enum {
	LOCK_NONE,
	LOCK_CASINO,
	LOCK_BOSS_KEY
} ExitLock;

typedef struct {
	Rectangle art;
	bool solid;
	float dmg;
} SceneHitbox;

typedef struct {
	Rectangle trigger;
	SceneId target;
	Vector2 arrival;
	ExitLock lock;
	DoorStyle doorStyle;
	Rectangle doorArea;
} SceneExit;

typedef struct {
	const char* name;
	const char* background;
	float artHeight;
	Vector2 spawn;
	int minionCount;
	bool hasDemon;
	Vector2 demonPos;
	bool isBossRoom;
	const SceneHitbox* hitboxes;
	int hitboxCount;
	SceneExit exits[MAX_SCENE_EXITS];
	int exitCount;
	const char* overlayFile;
	Vector2 overlayPos;
	float overlayBaseY;
} SceneDef;

void Scenes_LoadAssets(void);
void Scenes_UnloadAssets(void);

const SceneDef* Scene_Get(SceneId id);
Texture2D* Scene_Background(SceneId id);
float Scene_Scale(const SceneDef* def);
Rectangle Scene_RectToWorld(const SceneDef* def, Rectangle art);
Vector2 Scene_PointToWorld(const SceneDef* def, Vector2 art);
void Scene_LoadHitboxes(const SceneDef* def, LoadedAssetsClass* Assets);
void Scene_InitDoors(const SceneDef* def, Door doors[MAX_SCENE_EXITS]);
void Scene_DrawOverlay(SceneId id, float plrFeetY);

#endif
