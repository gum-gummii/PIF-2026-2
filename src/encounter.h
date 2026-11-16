#ifndef ENCOUNTER_H
#define ENCOUNTER_H

#include <raylib.h>

typedef struct {
	bool active;
	float t;
	Vector2 soulStart;
	char text[64];
} Encounter;

void Encounter_Start(Encounter* enc, Vector2 plrScreenCenter, const char* enemyName);
void Encounter_Update(Encounter* enc, float dt);
bool Encounter_IsActive(const Encounter* enc);
void Encounter_Draw(const Encounter* enc);
void Encounter_DrawSoul(Vector2 center, Color color);

#endif
