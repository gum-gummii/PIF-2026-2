#include <scenes.h>
#include <scene_overlay.h>
#include <stddef.h>

#define COUNT(a) ((int)(sizeof(a) / sizeof((a)[0])))
#define SOLID(x, y, w, h) {{x, y, w, h}, true, 0}

static const SceneHitbox GARDEN_HITBOXES[] = {
	SOLID(0, 366, 297, 297),
	{{0, 0, 148, 148}, false, 10},
	{{1068, 0, 198, 198}, false, -25},
};

static const SceneHitbox PARKING_HITBOXES[] = {
	SOLID(0, 0, 3256, 115),
	SOLID(15, 600, 180, 150),
	SOLID(225, 640, 175, 110),
	SOLID(500, 620, 175, 130),
	SOLID(1065, 620, 180, 130),
	SOLID(2165, 330, 230, 95),
};

static const SceneHitbox ENTRANCE_HITBOXES[] = {
	SOLID(0, 0, 1412, 110),
	SOLID(1125, 0, 287, 748),
	SOLID(1080, 0, 120, 240),
};

static const SceneHitbox ROOM1_HITBOXES[] = {
	SOLID(0, 0, 45, 90),
	SOLID(0, 0, 490, 70),
	SOLID(405, 0, 85, 85),
	SOLID(880, 0, 532, 90),
	SOLID(1335, 0, 77, 105),
	SOLID(0, 705, 1412, 43),
	SOLID(0, 685, 60, 63),
	SOLID(400, 690, 95, 58),
	SOLID(855, 688, 70, 60),
	SOLID(1355, 680, 57, 68),
};

static const SceneHitbox ROOM2_HITBOXES[] = {
	SOLID(0, 0, 45, 90),
	SOLID(0, 0, 490, 70),
	SOLID(405, 0, 85, 85),
	SOLID(490, 0, 400, 80),
	SOLID(875, 0, 70, 100),
	SOLID(945, 0, 467, 100),
	SOLID(930, 100, 482, 40),
	SOLID(990, 140, 422, 55),
	SOLID(1045, 195, 367, 25),
	SOLID(1100, 220, 312, 40),
	SOLID(1160, 260, 252, 40),
	SOLID(1205, 300, 85, 18),
	SOLID(0, 705, 900, 43),
	SOLID(0, 685, 60, 63),
	SOLID(400, 690, 95, 58),
	SOLID(855, 688, 70, 60),
	SOLID(900, 690, 512, 58),
	SOLID(960, 660, 452, 30),
	SOLID(1020, 630, 392, 30),
	SOLID(1045, 580, 367, 50),
};

static const SceneHitbox KEY_ROOM_HITBOXES[] = {
	SOLID(0, 0, 1412, 95),
	SOLID(1335, 0, 77, 110),
	SOLID(0, 0, 25, 748),
	SOLID(1390, 0, 22, 748),
	SOLID(0, 705, 490, 43),
	SOLID(0, 685, 60, 63),
	SOLID(400, 690, 95, 58),
	SOLID(860, 705, 552, 43),
	SOLID(855, 688, 70, 60),
	SOLID(1355, 680, 57, 68),
};

static const SceneHitbox BOSS_ROOM_HITBOXES[] = {
	SOLID(0, 0, 1837, 75),
	SOLID(0, 75, 520, 40),
	SOLID(0, 115, 440, 50),
	SOLID(0, 165, 380, 50),
	SOLID(0, 215, 300, 50),
	SOLID(0, 265, 210, 45),
	SOLID(925, 0, 75, 80),
	SOLID(1370, 0, 75, 90),
	SOLID(0, 585, 300, 163),
	SOLID(285, 580, 80, 168),
	SOLID(360, 625, 60, 123),
	SOLID(420, 655, 60, 93),
	SOLID(480, 685, 70, 63),
	SOLID(0, 705, 1837, 43),
	SOLID(925, 688, 80, 60),
	SOLID(1355, 685, 80, 63),
	SOLID(1775, 680, 62, 68),
	SOLID(1800, 0, 37, 748),
};

static const SceneDef SCENES[SCENE_COUNT] = {
	[SCENE_GARDEN] = {
		.name = "Garden",
		.background = "kardkassino_garden.png",
		.artHeight = 712,
		.spawn = {687, 488},
		.minionCount = 7,
		.hitboxes = GARDEN_HITBOXES,
		.hitboxCount = COUNT(GARDEN_HITBOXES),
	},
	[SCENE_PARKING] = {
		.name = "Parking",
		.background = "kardkassino_parking.png",
		.artHeight = 750,
		.spawn = {120, 400},
		.minionCount = 5,
		.hitboxes = PARKING_HITBOXES,
		.hitboxCount = COUNT(PARKING_HITBOXES),
		.exits = {
			{{3210, 0, 46, 750}, SCENE_ENTRANCE, {90, 400}, LOCK_NONE},
		},
		.exitCount = 1,
		.overlayFile = "kardkassino_parking_fountain.png",
		.overlayPos = {2207, 172},
		.overlayBaseY = 380,
	},
	[SCENE_ENTRANCE] = {
		.name = "Entrance",
		.background = "kardkassino_entrace.png",
		.artHeight = 748,
		.spawn = {90, 400},
		.minionCount = 3,
		.hitboxes = ENTRANCE_HITBOXES,
		.hitboxCount = COUNT(ENTRANCE_HITBOXES),
		.exits = {
			{{0, 0, 46, 748}, SCENE_PARKING, {3140, 400}, LOCK_NONE},
			{{1040, 330, 90, 130}, SCENE_ROOM1, {90, 410}, LOCK_CASINO, DOOR_PORTAL, {1115, 330, 75, 130}},
		},
		.exitCount = 2,
	},
	[SCENE_ROOM1] = {
		.name = "Room1",
		.background = "karddkassino_room1.png",
		.artHeight = 748,
		.spawn = {90, 410},
		.minionCount = 2,
		.hitboxes = ROOM1_HITBOXES,
		.hitboxCount = COUNT(ROOM1_HITBOXES),
		.exits = {
			{{0, 90, 46, 600}, SCENE_ENTRANCE, {1000, 400}, LOCK_NONE},
			{{490, 0, 390, 150}, SCENE_KEY_ROOM, {675, 640}, LOCK_NONE},
			{{1366, 90, 46, 600}, SCENE_ROOM2, {90, 410}, LOCK_NONE},
		},
		.exitCount = 3,
	},
	[SCENE_ROOM2] = {
		.name = "Room2",
		.background = "karddkassino_room2.png",
		.artHeight = 748,
		.spawn = {90, 410},
		.minionCount = 2,
		.hitboxes = ROOM2_HITBOXES,
		.hitboxCount = COUNT(ROOM2_HITBOXES),
		.exits = {
			{{0, 90, 46, 600}, SCENE_ROOM1, {1320, 410}, LOCK_NONE},
			{{1366, 300, 46, 285}, SCENE_BOSS_ROOM, {90, 440}, LOCK_BOSS_KEY, DOOR_GATE, {1385, 300, 27, 285}},
		},
		.exitCount = 2,
	},
	[SCENE_KEY_ROOM] = {
		.name = "KeyRoom",
		.background = "karddkassino_keyroom.png",
		.artHeight = 748,
		.spawn = {675, 600},
		.minionCount = 0,
		.hasDemon = true,
		.demonPos = {705, 330},
		.hitboxes = KEY_ROOM_HITBOXES,
		.hitboxCount = COUNT(KEY_ROOM_HITBOXES),
		.exits = {
			{{490, 690, 370, 58}, SCENE_ROOM1, {685, 230}, LOCK_NONE},
		},
		.exitCount = 1,
	},
	[SCENE_BOSS_ROOM] = {
		.name = "BossRoom",
		.background = "karddkassino_bossroom.png",
		.artHeight = 748,
		.spawn = {90, 440},
		.minionCount = 0,
		.isBossRoom = true,
		.hitboxes = BOSS_ROOM_HITBOXES,
		.hitboxCount = COUNT(BOSS_ROOM_HITBOXES),
		.exits = {
			{{0, 300, 46, 290}, SCENE_ROOM2, {1320, 440}, LOCK_NONE},
		},
		.exitCount = 1,
	},
};

static Texture2D backgrounds[SCENE_COUNT];
static SceneOverlay overlays[SCENE_COUNT];

void Scenes_LoadAssets(void) {
	for (int i = 0; i < SCENE_COUNT; i++) {
		backgrounds[i] = LoadTexture(SCENES[i].background);
		if (SCENES[i].overlayFile != NULL) {
			overlays[i] = SceneOverlay_Load(SCENES[i].overlayFile, SCENES[i].overlayPos, SCENES[i].overlayBaseY);
		}
	}
}

void Scenes_UnloadAssets(void) {
	for (int i = 0; i < SCENE_COUNT; i++) {
		if (backgrounds[i].id != 0) { UnloadTexture(backgrounds[i]); }
		SceneOverlay_Unload(&overlays[i]);
	}
}

const SceneDef* Scene_Get(SceneId id) {
	if (id < 0 || id >= SCENE_COUNT) { return &SCENES[SCENE_PARKING]; }
	return &SCENES[id];
}

Texture2D* Scene_Background(SceneId id) {
	if (id < 0 || id >= SCENE_COUNT || backgrounds[id].id == 0) { return NULL; }
	return &backgrounds[id];
}

float Scene_Scale(const SceneDef* def) {
	return GetScreenHeight() / def->artHeight;
}

Rectangle Scene_RectToWorld(const SceneDef* def, Rectangle art) {
	float k = Scene_Scale(def);
	return (Rectangle){art.x * k, art.y * k, art.width * k, art.height * k};
}

Vector2 Scene_PointToWorld(const SceneDef* def, Vector2 art) {
	float k = Scene_Scale(def);
	return (Vector2){art.x * k, art.y * k};
}

void Scene_LoadHitboxes(const SceneDef* def, LoadedAssetsClass* Assets) {
	Assets->FreeHitboxes(Assets);
	Assets->FreeImages(Assets);
	for (int i = 0; i < def->hitboxCount; i++) {
		const SceneHitbox* h = &def->hitboxes[i];
		Assets->NewHitbox(Assets, (char*)def->name, Scene_RectToWorld(def, h->art), h->solid, h->dmg);
	}
}

void Scene_InitDoors(const SceneDef* def, Door doors[MAX_SCENE_EXITS]) {
	for (int i = 0; i < def->exitCount; i++) {
		const SceneExit* e = &def->exits[i];
		Door_Init(&doors[i], e->doorStyle, Scene_RectToWorld(def, e->doorArea), Scene_RectToWorld(def, e->trigger));
	}
}

void Scene_DrawOverlay(SceneId id, float plrFeetY) {
	const SceneDef* def = Scene_Get(id);
	if (def->overlayFile == NULL) { return; }
	SceneOverlay_DrawIfInFront(&overlays[id], plrFeetY, Scene_Scale(def));
}
