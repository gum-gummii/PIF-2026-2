#include <score.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void Score_Init(Score* s, const char* filePath) {
	memset(s, 0, sizeof(*s));
	snprintf(s->filePath, sizeof(s->filePath), "%s", filePath);

	if (FileExists(s->filePath)) {
		char* txt = LoadFileText(s->filePath);
		if (txt != NULL) {
			s->best = atoi(txt);
			if (s->best < 0) { s->best = 0; }
			UnloadFileText(txt);
		}
	}
}

void Score_Reset(Score* s) {
	s->points = 0;
	s->kills = 0;
	s->combo = 0;
	s->comboTimer = 0;
	s->bumpT = 0;
	s->newBest = false;
	for (int i = 0; i < MAX_SCORE_POPUPS; i++) { s->popups[i].active = false; }
}

void Score_Save(const Score* s) {
	char buf[32];
	snprintf(buf, sizeof(buf), "%d", s->best);
	if (!SaveFileText(s->filePath, buf)) {
		printf("[score] nao consegui salvar em %s\n", s->filePath);
	}
}

static void SpawnPopup(Score* s, Vector2 pos, const char* text, Color color) {
	int slot = 0;
	float lowest = 999;
	for (int i = 0; i < MAX_SCORE_POPUPS; i++) {
		if (!s->popups[i].active) { slot = i; break; }
		if (s->popups[i].life < lowest) { lowest = s->popups[i].life; slot = i; }
	}

	ScorePopup* p = &s->popups[slot];
	p->active = true;
	p->pos = pos;
	p->life = 1.0f;
	p->color = color;
	snprintf(p->text, sizeof(p->text), "%s", text);
}

void Score_AddKill(Score* s, int basePoints, Vector2 worldPos) {
	s->combo = (s->comboTimer > 0) ? s->combo + 1 : 1;
	if (s->combo > COMBO_MAX) { s->combo = COMBO_MAX; }
	s->comboTimer = COMBO_WINDOW;

	int gained = basePoints * s->combo;
	s->points += gained;
	s->kills++;
	s->bumpT = 1.0f;

	if (s->points > s->best) {
		s->best = s->points;
		s->newBest = true;
	}

	char txt[24];
	if (s->combo > 1) { snprintf(txt, sizeof(txt), "+%d x%d", gained, s->combo); }
	else              { snprintf(txt, sizeof(txt), "+%d", gained); }

	Color c = (s->combo >= 4) ? ORANGE : (s->combo >= 2 ? YELLOW : WHITE);
	SpawnPopup(s, worldPos, txt, c);
}

void Score_Update(Score* s, float dt) {
	if (s->comboTimer > 0) {
		s->comboTimer -= dt;
		if (s->comboTimer <= 0) { s->comboTimer = 0; s->combo = 0; }
	}
	if (s->bumpT > 0) { s->bumpT -= dt * 4; if (s->bumpT < 0) { s->bumpT = 0; } }

	for (int i = 0; i < MAX_SCORE_POPUPS; i++) {
		ScorePopup* p = &s->popups[i];
		if (!p->active) { continue; }
		p->life -= dt;
		p->pos.y -= 45 * dt;
		if (p->life <= 0) { p->active = false; }
	}
}

void Score_DrawPopups(const Score* s) {
	for (int i = 0; i < MAX_SCORE_POPUPS; i++) {
		const ScorePopup* p = &s->popups[i];
		if (!p->active) { continue; }

		int fs = 24;
		int w = MeasureText(p->text, fs);
		float a = p->life < 0.3f ? p->life / 0.3f : 1.0f;
		DrawText(p->text, (int)p->pos.x - w / 2 + 2, (int)p->pos.y + 2, fs, Fade(BLACK, a));
		DrawText(p->text, (int)p->pos.x - w / 2, (int)p->pos.y, fs, Fade(p->color, a));
	}
}

void Score_DrawHud(const Score* s) {
	int sw = GetScreenWidth();
	int x = sw - 260;
	int y = 15;

	DrawRectangle(x - 10, y - 5, 255, s->combo > 1 ? 110 : 80, Fade(BLACK, 0.55f));

	int fs = 30 + (int)(s->bumpT * 8);
	DrawText(TextFormat("PONTOS: %d", s->points), x, y, fs, WHITE);
	DrawText(TextFormat("RECORDE: %d%s", s->best, s->newBest ? " (NOVO!)" : ""), x, y + 38, 18, s->newBest ? GOLD : LIGHTGRAY);
	DrawText(TextFormat("abates: %d", s->kills), x, y + 58, 16, GRAY);

	if (s->combo > 1) {
		DrawText(TextFormat("COMBO x%d", s->combo), x, y + 78, 20, s->combo >= 4 ? ORANGE : YELLOW);
		float k = s->comboTimer / COMBO_WINDOW;
		DrawRectangle(x + 120, y + 84, (int)(110 * k), 8, YELLOW);
	}
}
