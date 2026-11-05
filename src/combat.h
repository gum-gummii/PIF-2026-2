#ifndef COMBAT_H
#define COMBAT_H

#include <raylib.h>
#include <classes.h>

#define PLAYER_MAX_HP     100
#define MAX_PROJECTILES   32
#define MAX_SLASHES       4

typedef enum {
	ENEMY_CHASE,
	ENEMY_MELEE_WINDUP,
	ENEMY_MELEE_STRIKE,
	ENEMY_SHOOT_WINDUP,
	ENEMY_RECOVER
} EnemyState;

typedef struct {
	bool active;
	bool fromPlayer;
	Vector2 pos;      
	Vector2 vel;      
	float size;
	float life;
	int dmg;
} Projectile;

typedef struct {
	bool active;
	Vector2 dir;     
	float life;
	bool hitDone;
	int dmg;
} Slash;

typedef struct {
	bool present;     
	bool alive;
	Vector2 pos;      
	Vector2 knockback;
	Vector2 strikeDir;
	bool facingRight;
	int hp;
	int maxHp;
	EnemyState state;
	float stateTimer;
	float attackCd;
	float hitFlash;
	bool strikeHit;
} Enemy;

typedef struct {
	Enemy enemy;
	Projectile projectiles[MAX_PROJECTILES];
	Slash slashes[MAX_SLASHES];

	Rectangle lastPlayerBody;  
	Vector2 facing;   
	float meleeCd;
	float shootCd;
	float invuln;     
	float time;
} CombatState;

void Combat_Init(CombatState* c);
void Combat_SpawnEnemy(CombatState* c, Vector2 pos);   
void Combat_ClearEnemy(CombatState* c);                
void Combat_Update(CombatState* c, float dt, Rectangle plrBody, int* plrHp, Vector2 worldSize, LoadedAssetsClass* Assets, bool allowInput);
void Combat_Draw(CombatState* c);                      
void Combat_DrawHud(CombatState* c, int plrHp);       
bool Combat_PlayerBlink(CombatState* c);               

#endif
