#ifndef PROGRESSION_H
#define PROGRESSION_H

#include <raylib.h>
#include <battle.h>
#include <minions.h>
#include <score.h>

typedef struct {
	int wins;
	int winsToBoss;
	bool bossPending;
	bool bossOnMap;
	bool cleared;
	int fighting;
	float grace;
	float bannerT;
	char banner[64];
} Progression;

typedef struct {
	MinionField* minions;
	Score* score;
	Vector2 worldSize;
	Rectangle plrBody;
	LoadedAssetsClass* Assets;
} StageContext;

void Progression_Reset(Progression* p, int winsToBoss);
bool Progression_CanEncounter(const Progression* p);
void Progression_BeginFight(Progression* p, int minionIndex);
void Progression_EndFight(Progression* p, BattleOutcome outcome, StageContext ctx);
void Progression_Update(Progression* p, float dt, StageContext ctx);
void Progression_DrawHud(const Progression* p);

#endif
