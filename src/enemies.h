#ifndef ENEMIES_H
#define ENEMIES_H

#include <raylib.h>

#define ENEMY_ACTS 2

typedef enum {
	ENEMY_DEMONINHO,
	ENEMY_GOSMA,
	ENEMY_MORCEGO,
	ENEMY_TANQUE,
	ENEMY_TYPE_COUNT
} EnemyType;

typedef enum {
	PATTERN_RAIN  = 1 << 0,
	PATTERN_SIDES = 1 << 1,
	PATTERN_FAN   = 1 << 2
} BulletPattern;

typedef struct {
	const char* label;
	float mercy;
	const char* text;
} EnemyAct;

typedef struct {
	EnemyType type;
	const char* name;
	const char* article;
	int maxHp;
	float size;
	Color color;

	float bulletSpeed;
	float damageMult;
	float turnTime;
	unsigned patterns;

	bool isBoss;
	int exp;
	int gold;
	int points;

	const char* checkText;
	const char* firstFlavor;
	const char* spareFlavor;
	const char* angryFlavor;
	const char* flavor[3];
	const char* linesNormal[3];
	const char* linesCalm[3];
	const char* linesAngry[3];
	EnemyAct acts[ENEMY_ACTS];
} EnemyDef;

typedef struct {
	bool flash;
	bool angry;
	bool calm;
	bool alert;
} EnemyLook;

const EnemyDef* Enemy_Get(EnemyType type);
void Enemy_Draw(const EnemyDef* def, Vector2 center, float size, float bob, EnemyLook look, float alpha);

#endif
