#ifndef SCORE_H
#define SCORE_H

#include <raylib.h>

#define MAX_SCORE_POPUPS  16
#define COMBO_WINDOW      2.0f
#define COMBO_MAX         5

typedef struct {
	bool active;
	Vector2 pos;
	float life;
	char text[24];
	Color color;
} ScorePopup;

typedef struct {
	int points;
	int best;
	int kills;
	int combo;
	float comboTimer;
	float bumpT;
	bool newBest;
	char filePath[128];
	ScorePopup popups[MAX_SCORE_POPUPS];
} Score;

void Score_Init(Score* s, const char* filePath);
void Score_Reset(Score* s);
void Score_AddKill(Score* s, int basePoints, Vector2 worldPos);
void Score_Update(Score* s, float dt);
void Score_Save(const Score* s);

void Score_DrawPopups(const Score* s);
void Score_DrawHud(const Score* s);

#endif
