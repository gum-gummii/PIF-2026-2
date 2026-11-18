#ifndef MINIONS_H
#define MINIONS_H

#include <raylib.h>
#include <classes.h>
#include <score.h>

#define MAX_MINIONS 16

typedef enum {
	MINION_SLIME,
	MINION_BAT,
	MINION_TANK,
	MINION_KIND_COUNT
} MinionKind;

typedef struct {
	bool alive;
	MinionKind kind;
	Vector2 pos;
	Vector2 knock;
	Vector2 wanderDir;
	float wanderT;
	float size;
	int hp;
	int maxHp;
	float hitFlash;
	float respawnT;
	float deathT;
	float bob;
	int lastSwingHit;
} Minion;

typedef struct {
	Minion m[MAX_MINIONS];
	int count;

	float attackT;
	float attackCd;
	Vector2 attackDir;
	int swingId;
	Rectangle attackRect;

	float contactCd;
	float hurtFlash;
} MinionField;

void Minions_Spawn(MinionField* f, int count, Vector2 worldSize, Rectangle plrBody, LoadedAssetsClass* Assets);
void Minions_Clear(MinionField* f);

void Minions_Update(MinionField* f, float dt, Rectangle plrBody, Vector2 facing, bool attackPressed,
                    int* plrHp, Score* score, Vector2 worldSize, LoadedAssetsClass* Assets);

void Minions_Draw(const MinionField* f);
void Minions_DrawHurtOverlay(const MinionField* f);
bool Minions_PlayerBlink(const MinionField* f);

#endif
