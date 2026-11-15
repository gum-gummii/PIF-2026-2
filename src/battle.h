#ifndef BATTLE_H
#define BATTLE_H

#include <raylib.h>
#include <combat.h>

#define BUTTON_COUNT   5
#define ACT_COUNT      4
#define ITEM_SLOTS     3
#define MAX_BULLETS    128
#define MAX_SHARDS     8

typedef enum {
	B_FADE_IN,
	B_MENU,
	B_FIGHT_SELECT,
	B_ACT_SELECT,
	B_ACT_LIST,
	B_ITEM_LIST,
	B_SPARE_LIST,
	B_ATTACK,
	B_ATTACK_ANIM,
	B_TEXT,
	B_ENEMY_DIALOGUE,
	B_ENEMY_ATTACK,
	B_LOST,
	B_DONE
} BattleState;

typedef struct {
	bool active;
	Vector2 pos;
	Vector2 vel;
	float r;
	int dmg;
	bool grazed;
} Bullet;

typedef struct {
	const char* name;
	int heal;
	int count;
} BattleItem;

typedef struct {
	int hp;
	int maxHp;
	float mercy;
	float hitFlash;
	float shake;
} BattleEnemy;

typedef struct {
	bool active;
	BattleState state;
	float stateT;
	float time;
	float menuLock;

	int menuIndex;
	int listIndex;
	int turn;
	char flavor[128];

	BattleEnemy enemy;
	float tp;
	bool defending;
	int outcome;
	float outcomeT;

	char text[192];
	float textT;
	BattleState afterText;

	float attackP;
	float attackAcc;
	bool attackMissed;
	bool dmgApplied;
	int lastDamage;
	float dmgNumT;

	Rectangle box;
	char line[64];
	int pattern;
	int lastPattern;
	float spawnT;
	Bullet bullets[MAX_BULLETS];

	Vector2 soul;
	float invuln;
	float hurtFlash;
	Vector2 shards[MAX_SHARDS];
	Vector2 shardVel[MAX_SHARDS];

	BattleItem items[ITEM_SLOTS];
	bool restartRequested;

	Texture2D plrTexture;
	Rectangle plrSrc;
} Battle;

void Battle_Start(Battle* b, Texture2D plrTexture, Rectangle plrSrc);
void Battle_Stop(Battle* b);
void Battle_Update(Battle* b, float dt, int* plrHp);
void Battle_Draw(const Battle* b, int plrHp);
bool Battle_IsActive(const Battle* b);
bool Battle_ConsumeRestart(Battle* b);

#endif
