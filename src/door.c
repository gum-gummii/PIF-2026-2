#include <door.h>
#include <math.h>
#include <stddef.h>

#define OPEN_TIME   0.8f
#define NEAR_RANGE  260.0f
#define GATE_BAR_GAP 9.0f

#define INTERIOR_COLOR (Color){34, 12, 48, 255}
#define GLOW_COLOR     (Color){255, 190, 110, 255}
#define LOCK_COLOR     (Color){240, 190, 70, 255}
#define GATE_DARK      (Color){120, 80, 20, 255}

static Vector2 RectCenter(Rectangle r) { return (Vector2){r.x + r.width / 2, r.y + r.height / 2}; }

static float EaseOut(float t) { float i = 1.0f - t; return 1.0f - i * i * i; }

void Door_Init(Door* door, DoorStyle style, Rectangle area, Rectangle entry) {
	*door = (Door){0};
	door->style = style;
	door->area = area;
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

bool Door_IsOpen(const Door* door) {
	return door->openT >= 1.0f;
}

static void DrawPortal(const Door* door) {
	if (door->openT <= 0) { return; }

	Vector2 c = RectCenter(door->area);
	float k = EaseOut(door->openT);
	float rx = door->area.width / 2 * k, ry = door->area.height / 2 * k;
	float pulse = 0.5f + 0.5f * sinf(door->time * 3.0f);

	DrawEllipse((int)c.x, (int)c.y, rx * 1.25f, ry * 1.1f, Fade(GLOW_COLOR, 0.25f + 0.15f * pulse));
	DrawEllipse((int)c.x, (int)c.y, rx, ry, INTERIOR_COLOR);
	DrawEllipse((int)c.x, (int)(c.y + ry * 0.35f), rx * 0.6f, ry * 0.45f, Fade(GLOW_COLOR, 0.35f + 0.2f * pulse));
	DrawEllipseLines((int)c.x, (int)c.y, rx, ry, BLACK);
}

static void DrawGate(const Door* door) {
	float visible = door->area.height * (1.0f - EaseOut(door->openT));
	if (visible < 1.0f) { return; }

	Rectangle r = {door->area.x, door->area.y, door->area.width, visible};
	DrawRectangleRec(r, Fade(BLACK, 0.35f));
	for (float x = r.x + 3; x < r.x + r.width - 2; x += GATE_BAR_GAP) {
		DrawRectangle((int)x - 1, (int)r.y, 6, (int)r.height, GATE_DARK);
		DrawRectangle((int)x, (int)r.y, 4, (int)r.height, LOCK_COLOR);
	}
	DrawRectangle((int)r.x, (int)(r.y + r.height - 8), (int)r.width, 8, GATE_DARK);
	DrawRectangleLinesEx(r, 2, BLACK);
}

static void DrawLock(Vector2 c) {
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
	Vector2 c = RectCenter(door->entry);
	int x = (int)c.x - w / 2, y = (int)(door->area.y - 50);
	if (x + w + 20 > door->area.x + door->area.width) { x = (int)(door->area.x + door->area.width) - w - 20; }
	DrawRectangle(x - 10, y - 6, w + 20, fs + 12, Fade(BLACK, 0.75f));
	DrawText(text, x, y, fs, LOCK_COLOR);
}

void Door_Draw(const Door* door, const char* lockedHint) {
	if (door->style == DOOR_PORTAL) { DrawPortal(door); }
	else { DrawGate(door); }

	if (door->unlocked) { return; }
	DrawLock(RectCenter(door->area));
	if (door->playerNear && lockedHint != NULL) { DrawHint(door, lockedHint); }
}
