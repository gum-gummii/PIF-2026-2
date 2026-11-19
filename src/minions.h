#ifndef MINIONS_H
#define MINIONS_H

#include <raylib.h>
#include <classes.h>
#include <score.h>
#include <enemies.h>

#define MAX_MINIONS 16

typedef enum {
	MINION_SLIME,
	MINION_BAT,
	MINION_TANK,
	MINION_BOSS,
	MINION_KIND_COUNT
} MinionKind;

typedef struct {
	bool alive;
	bool permanent;
	bool chasing;
	MinionKind kind;
	Vector2 pos;
	Vector2 knock;
	Vector2 wanderDir;
	float wanderT;
	float alertT;
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
} MinionField;

typedef struct {
	Rectangle plrBody;
	Vector2 facing;
	bool attackPressed;
	bool plrAlive;
	bool allowEncounter;
} MinionInput;

void Minions_Spawn(MinionField* f, int count, Vector2 worldSize, Rectangle plrBody, LoadedAssetsClass* Assets);
void Minions_Clear(MinionField* f);
int  Minions_SpawnBoss(MinionField* f, Vector2 pos);
void Minions_Remove(MinionField* f, int index);

int Minions_Update(MinionField* f, float dt, MinionInput in, Score* score, Vector2 worldSize, LoadedAssetsClass* Assets);

EnemyType Minions_EnemyOf(const MinionField* f, int index);
Vector2   Minions_Position(const MinionField* f, int index);

void Minions_Draw(const MinionField* f);

#endif
