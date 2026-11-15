#include <encounter.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define T_EXCLAIM_END   0.55f
#define T_DIM_END       (T_EXCLAIM_END + 0.30f)
#define T_BLINK_END     (T_DIM_END + 0.55f)
#define T_FLY_END       (T_BLINK_END + 0.50f)
#define TEXT_SPEED      30.0f
#define T_HOLD          0.8f
#define T_FADE_OUT      0.35f

#define OVERLAY_MAX     0.82f
#define OVERLAY_BATTLE  0.55f
#define SOUL_PIXEL      3

static const char* SOUL_SPRITE[8] = {
	".XX...XX.",
	"XXXX.XXXX",
	"XXXXXXXXX",
	"XXXXXXXXX",
	".XXXXXXX.",
	"..XXXXX..",
	"...XXX...",
	"....X....",
};

static float Clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
static float EaseOutCubic(float x) { float i = 1.0f - x; return 1.0f - i * i * i; }

static float TextEndTime(const Encounter* enc) {
	return T_FLY_END + 0.15f + (float)strlen(enc->text) / TEXT_SPEED;
}

static float TotalTime(const Encounter* enc) {
	return TextEndTime(enc) + T_HOLD + T_FADE_OUT;
}

static Vector2 SoulTarget(void) {
	return (Vector2){110, GetScreenHeight() - 140};
}

void Encounter_DrawSoul(Vector2 center, Color color) {
	float w = 9 * SOUL_PIXEL, h = 8 * SOUL_PIXEL;
	for (int row = 0; row < 8; row++) {
		for (int col = 0; col < 9; col++) {
			if (SOUL_SPRITE[row][col] != 'X') { continue; }
			DrawRectangle((int)(center.x - w / 2) + col * SOUL_PIXEL, (int)(center.y - h / 2) + row * SOUL_PIXEL, SOUL_PIXEL, SOUL_PIXEL, color);
		}
	}
}

static Vector2 SoulPosAt(const Encounter* enc, float t) {
	if (t <= T_BLINK_END) { return enc->soulStart; }
	float k = EaseOutCubic(Clamp01((t - T_BLINK_END) / (T_FLY_END - T_BLINK_END)));
	Vector2 to = SoulTarget();
	return (Vector2){enc->soulStart.x + (to.x - enc->soulStart.x) * k, enc->soulStart.y + (to.y - enc->soulStart.y) * k};
}

void Encounter_Start(Encounter* enc, Vector2 plrScreenCenter, const char* enemyName) {
	enc->active = true;
	enc->t = 0;
	enc->soulStart = plrScreenCenter;
	snprintf(enc->text, sizeof(enc->text), "* %s apareceu!", enemyName);
}

void Encounter_Update(Encounter* enc, float dt) {
	if (!enc->active) { return; }
	enc->t += dt;
	if (IsKeyPressed(KEY_ENTER) || enc->t >= TotalTime(enc)) { enc->active = false; }
}

bool Encounter_IsActive(const Encounter* enc) { return enc->active; }

void Encounter_Draw(const Encounter* enc) {
	if (!enc->active) { return; }

	int sw = GetScreenWidth(), sh = GetScreenHeight();
	float t = enc->t;

	float overlay = 0;
	if (t >= T_EXCLAIM_END && t < T_DIM_END) {
		overlay = OVERLAY_MAX * Clamp01((t - T_EXCLAIM_END) / (T_DIM_END - T_EXCLAIM_END));
	} else if (t >= T_DIM_END && t < T_FLY_END) {
		overlay = OVERLAY_MAX;
	} else if (t >= T_FLY_END) {
		float toBattle = Clamp01((t - T_FLY_END) / 0.4f);
		overlay = OVERLAY_MAX + (OVERLAY_BATTLE - OVERLAY_MAX) * toBattle;
	}
	DrawRectangle(0, 0, sw, sh, Fade((Color){8, 0, 20, 255}, overlay));

	if (t >= T_DIM_END) {
		float gridAlpha = overlay / OVERLAY_MAX * 0.35f;
		float scroll = fmodf(t * 40.0f, 40.0f);
		for (int x = -40; x < sw + 40; x += 40) { DrawLineEx((Vector2){x + scroll, 0}, (Vector2){x + scroll, (float)sh}, 2, Fade((Color){160, 60, 220, 255}, gridAlpha)); }
		for (int y = -40; y < sh + 40; y += 40) { DrawLineEx((Vector2){0, y + scroll}, (Vector2){(float)sw, y + scroll}, 2, Fade((Color){160, 60, 220, 255}, gridAlpha)); }
	}

	if (t >= T_EXCLAIM_END && t < T_EXCLAIM_END + 0.12f) {
		DrawRectangle(0, 0, sw, sh, Fade(WHITE, 0.8f * (1.0f - (t - T_EXCLAIM_END) / 0.12f)));
	}

	if (t < T_EXCLAIM_END + 0.1f) {
		float bounce = (t < 0.15f) ? (1.0f - t / 0.15f) * 10.0f : 0;
		const char* ex = "!";
		int fs = 56;
		DrawText(ex, (int)(enc->soulStart.x - MeasureText(ex, fs) / 2 + 3), (int)(enc->soulStart.y - 100 - bounce + 3), fs, BLACK);
		DrawText(ex, (int)(enc->soulStart.x - MeasureText(ex, fs) / 2), (int)(enc->soulStart.y - 100 - bounce), fs, RED);
	}

	if (t >= T_DIM_END) {
		if (t < T_BLINK_END) {
			bool visible = ((int)((t - T_DIM_END) / 0.1f) % 2) == 0;
			if (visible) { Encounter_DrawSoul(enc->soulStart, RED); }
		} else {
			for (int i = 5; i >= 1; i--) {
				float tp = t - i * 0.03f;
				if (tp < T_BLINK_END) { continue; }
				Encounter_DrawSoul(SoulPosAt(enc, tp), Fade(RED, 0.5f - i * 0.08f));
			}
			Encounter_DrawSoul(SoulPosAt(enc, t), RED);
		}
	}

	if (t >= T_FLY_END) {
		float appear = Clamp01((t - T_FLY_END) / 0.15f);
		Rectangle box = {70, (float)sh - 200, (float)sw - 140, 140};
		DrawRectangleRec(box, Fade(BLACK, appear));
		DrawRectangleLinesEx(box, 5, Fade(WHITE, appear));
		DrawRectangleLinesEx((Rectangle){box.x + 5, box.y + 5, box.width - 10, box.height - 10}, 2, Fade((Color){160, 60, 220, 255}, appear));

		if (t >= T_FLY_END) { Encounter_DrawSoul(SoulTarget(), RED); }

		float typed = (t - T_FLY_END - 0.15f) * TEXT_SPEED;
		if (typed > 0) {
			int n = (int)typed;
			int len = (int)strlen(enc->text);
			if (n > len) { n = len; }
			DrawText(TextSubtext(enc->text, 0, n), 160, (int)box.y + 52, 36, WHITE);
		}
	}

	float fadeStart = TotalTime(enc) - T_FADE_OUT;
	if (t > fadeStart) { DrawRectangle(0, 0, sw, sh, Fade(BLACK, Clamp01((t - fadeStart) / T_FADE_OUT))); }

	if (t > 0.3f) { DrawText("[ENTER] pular", sw - 170, 15, 18, Fade(LIGHTGRAY, 0.6f)); }
}
