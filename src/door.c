#include <door.h>
#include <math.h>
#include <stddef.h>

#define OPEN_TIME   0.8f
#define NEAR_RANGE  260.0f

#define PANEL_COLOR    (Color){255, 236, 180, 255}
#define PANEL_EDGE     (Color){220, 170, 90, 255}
#define INTERIOR_COLOR (Color){34, 12, 48, 255}
#define LOCK_COLOR     (Color){240, 190, 70, 255}

static Vector2 RectCenter(Rectangle r) { return (Vector2){r.x + r.width / 2, r.y + r.height / 2}; }

static float EaseOut(float t) { float i = 1.0f - t; return 1.0f - i * i * i; }

void Door_Init(Door* door, Rectangle panels, Rectangle entry) {
	*door = (Door){0};
	door->panels = panels;
	door->entry = entry;
}

void Door_Update(Door* door, float dt, bool unlocked, Rectangle plrFeet) {
	door->time += dt;
	door->unlocked = unlocked;

	Vector2 a = RectCenter(plrFeet), b = RectCenter(door->entry);
	float dx = a.x - b.x, dy = a.y - b.y;
	door->playerNear = sqrtf(dx * dx + dy * dy) < NEAR_RANGE;

	if (door->unlocked && door->playerNear && door->openT < 1.0f) {
		door->openT = fminf(1.0f, door->openT + dt / OPEN_TIME);
	}
}

bool Door_IsEntering(const Door* door, Rectangle plrFeet) {
	return door->openT >= 1.0f && CheckCollisionRecs(door->entry, plrFeet);
}

static void DrawInterior(const Door* door) {
	Rectangle r = door->panels;
	DrawRectangleRec(r, INTERIOR_COLOR);
	float pulse = 0.5f + 0.5f * sinf(door->time * 3.0f);
	DrawRectangleGradientV((int)r.x, (int)r.y, (int)r.width, (int)r.height, Fade((Color){255, 200, 120, 255}, 0.15f + 0.15f * pulse), Fade((Color){255, 120, 80, 255}, 0.55f));
}

static void DrawPanels(const Door* door) {
	Rectangle r = door->panels;
	float half = r.width / 2 * (1.0f - EaseOut(door->openT));
	if (half < 1.0f) { return; }

	Rectangle left = {r.x, r.y, half, r.height};
	Rectangle right = {r.x + r.width - half, r.y, half, r.height};
	DrawRectangleRec(left, PANEL_COLOR);
	DrawRectangleRec(right, PANEL_COLOR);
	DrawRectangleLinesEx(left, 2, PANEL_EDGE);
	DrawRectangleLinesEx(right, 2, PANEL_EDGE);
}

static void DrawLock(const Door* door) {
	Vector2 c = RectCenter(door->panels);
	DrawRing((Vector2){c.x, c.y - 6}, 6, 10, 180, 360, 16, BLACK);
	DrawRing((Vector2){c.x, c.y - 6}, 7, 9, 180, 360, 16, LOCK_COLOR);
	Rectangle body = {c.x - 13, c.y - 6, 26, 20};
	DrawRectangleRounded(body, 0.3f, 4, LOCK_COLOR);
	DrawRectangleRoundedLinesEx(body, 0.3f, 4, 2, BLACK);
	DrawCircle((int)c.x, (int)c.y + 2, 3, BLACK);
}

static void DrawHint(const Door* door, const char* text) {
	int fs = 22;
	int w = MeasureText(text, fs);
	Vector2 c = RectCenter(door->panels);
	int x = (int)c.x - w / 2, y = (int)(door->panels.y - 120);
	DrawRectangle(x - 10, y - 6, w + 20, fs + 12, Fade(BLACK, 0.75f));
	DrawText(text, x, y, fs, LOCK_COLOR);
}

void Door_Draw(const Door* door, const char* lockedHint) {
	if (door->openT > 0) { DrawInterior(door); }
	DrawPanels(door);

	if (door->unlocked) { return; }
	DrawLock(door);
	if (door->playerNear && lockedHint != NULL) { DrawHint(door, lockedHint); }
}
