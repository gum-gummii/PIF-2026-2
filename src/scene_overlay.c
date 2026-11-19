#include <scene_overlay.h>

SceneOverlay SceneOverlay_Load(const char* file, Vector2 artPos, float artBaseY) {
	return (SceneOverlay){LoadTexture(file), artPos, artBaseY};
}

void SceneOverlay_Unload(SceneOverlay* overlay) {
	if (overlay->texture.id != 0) { UnloadTexture(overlay->texture); }
	overlay->texture.id = 0;
}

void SceneOverlay_DrawIfInFront(const SceneOverlay* overlay, float plrFeetY, float artScale) {
	if (overlay->texture.id == 0 || plrFeetY >= overlay->artBaseY * artScale) { return; }

	Texture2D t = overlay->texture;
	Rectangle src = {0, 0, (float)t.width, (float)t.height};
	Rectangle dest = {overlay->artPos.x * artScale, overlay->artPos.y * artScale, t.width * artScale, t.height * artScale};
	DrawTexturePro(t, src, dest, (Vector2){0, 0}, 0, WHITE);
}
