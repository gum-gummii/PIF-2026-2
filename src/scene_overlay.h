#ifndef SCENE_OVERLAY_H
#define SCENE_OVERLAY_H

#include <raylib.h>

typedef struct {
	Texture2D texture;
	Vector2 artPos;
	float artBaseY;
} SceneOverlay;

SceneOverlay SceneOverlay_Load(const char* file, Vector2 artPos, float artBaseY);
void SceneOverlay_Unload(SceneOverlay* overlay);
void SceneOverlay_DrawIfInFront(const SceneOverlay* overlay, float plrFeetY, float artScale);

#endif
