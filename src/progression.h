#ifndef PROGRESSION_H
#define PROGRESSION_H

#include <raylib.h>
#include <battle.h>
#include <minions.h>
#include <score.h>

typedef struct {
	int wins;
	int winsToUnlock;
	bool doorUnlocked;
	bool cleared;
	bool inBossRoom;
	int fighting;
	float grace;
	float bannerT;
	char banner[64];
} Progression;

typedef struct {
	MinionField* minions;
	Score* score;
} StageContext;

void Progression_Reset(Progression* p, int winsToUnlock);
bool Progression_CanEncounter(const Progression* p);
int  Progression_WinsLeft(const Progression* p);
void Progression_BeginFight(Progression* p, int minionIndex);
void Progression_EndFight(Progression* p, BattleOutcome outcome, StageContext ctx);
void Progression_SetInBossRoom(Progression* p, bool inBossRoom);
void Progression_Update(Progression* p, float dt);
void Progression_DrawHud(const Progression* p);

#endif
