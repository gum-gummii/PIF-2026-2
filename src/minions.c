#include <minions.h>
#include <sprites.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define ATTACK_DURATION   0.15f
#define ATTACK_COOLDOWN   0.30f
#define ATTACK_REACH      95.0f
#define RESPAWN_TIME      4.0f
#define AGGRO_RANGE       330.0f
#define MIN_SPAWN_DIST    300.0f
#define ALERT_TIME        0.6f

typedef struct {
	float size;
	int hp;
	float speed;
	int points;
	Color color;
	EnemyType enemy;
	bool boss;
} MinionStats;

static const MinionStats STATS[MINION_KIND_COUNT] = {
	[MINION_SLIME] = {34, 2,  70, 10, {90, 200, 90, 255},  ENEMY_GOSMA,     false},
	[MINION_BAT]   = {26, 1, 140, 15, {150, 80, 200, 255}, ENEMY_MORCEGO,   false},
	[MINION_TANK]  = {44, 4,  50, 30, {230, 130, 40, 255}, ENEMY_TANQUE,    false},
	[MINION_BOSS]  = {60, 1,  95,  0, {130, 70, 190, 255}, ENEMY_DEMONINHO, true},
};

static float Rand01(void) { return (float)GetRandomValue(0, 10000) / 10000.0f; }

static Vector2 Norm(Vector2 v) {
	float l = sqrtf(v.x * v.x + v.y * v.y);
	return (l > 0.0001f) ? (Vector2){v.x / l, v.y / l} : (Vector2){0, 0};
}

static Rectangle MinionRect(Vector2 pos, float size) {
	return (Rectangle){pos.x - size / 2, pos.y - size / 2, size, size};
}

static Vector2 RectCenter(Rectangle r) { return (Vector2){r.x + r.width / 2, r.y + r.height / 2}; }

static bool FindFreeSpot(Vector2* out, float size, Vector2 worldSize, Rectangle plrBody, LoadedAssetsClass* Assets) {
	Vector2 plrC = RectCenter(plrBody);
	for (int tries = 0; tries < 60; tries++) {
		Vector2 p = {
			size + Rand01() * (worldSize.x - size * 2),
			size + Rand01() * (worldSize.y - size * 2),
		};
		float dx = p.x - plrC.x, dy = p.y - plrC.y;
		if (dx * dx + dy * dy < MIN_SPAWN_DIST * MIN_SPAWN_DIST) { continue; }
		if (Assets != NULL && Assets->CheckColisions(Assets, MinionRect(p, size))) { continue; }
		*out = p;
		return true;
	}
	return false;
}

static MinionKind RandomKind(void) {
	int r = GetRandomValue(0, 99);
	if (r < 55) { return MINION_SLIME; }
	if (r < 85) { return MINION_BAT; }
	return MINION_TANK;
}

static void SetupMinion(Minion* m, MinionKind kind, Vector2 pos) {
	const MinionStats* st = &STATS[kind];
	memset(m, 0, sizeof(*m));
	m->alive = true;
	m->permanent = st->boss;
	m->kind = kind;
	m->pos = pos;
	m->size = st->size;
	m->hp = st->hp;
	m->maxHp = st->hp;
	m->bob = Rand01() * 6.28f;
	m->lastSwingHit = -1;
}

static void ReviveMinion(Minion* m, Vector2 worldSize, Rectangle plrBody, LoadedAssetsClass* Assets) {
	MinionKind kind = RandomKind();
	Vector2 pos;
	if (!FindFreeSpot(&pos, STATS[kind].size, worldSize, plrBody, Assets)) {
		m->respawnT = 1.0f;
		return;
	}
	SetupMinion(m, kind, pos);
}

void Minions_Clear(MinionField* f) {
	memset(f, 0, sizeof(*f));
}

void Minions_Spawn(MinionField* f, int count, Vector2 worldSize, Rectangle plrBody, LoadedAssetsClass* Assets) {
	Minions_Clear(f);
	if (count > MAX_MINIONS) { count = MAX_MINIONS; }
	f->count = count;
	f->attackDir = (Vector2){0, 1};
	for (int i = 0; i < count; i++) {
		ReviveMinion(&f->m[i], worldSize, plrBody, Assets);
	}
}

int Minions_SpawnBoss(MinionField* f, Vector2 worldSize, Rectangle plrBody, LoadedAssetsClass* Assets) {
	if (f->count >= MAX_MINIONS) { return -1; }

	Vector2 pos;
	if (!FindFreeSpot(&pos, STATS[MINION_BOSS].size, worldSize, plrBody, Assets)) { return -1; }

	int index = f->count++;
	SetupMinion(&f->m[index], MINION_BOSS, pos);
	f->m[index].alertT = ALERT_TIME;
	return index;
}

void Minions_Remove(MinionField* f, int index) {
	if (index < 0 || index >= f->count) { return; }
	Minion* m = &f->m[index];
	m->alive = false;
	m->chasing = false;
	m->deathT = 0.3f;
	m->respawnT = RESPAWN_TIME + Rand01() * 2.0f;
}

EnemyType Minions_EnemyOf(const MinionField* f, int index) {
	return STATS[f->m[index].kind].enemy;
}

Vector2 Minions_Position(const MinionField* f, int index) {
	return f->m[index].pos;
}

static Rectangle BuildAttackRect(Rectangle plrBody, Vector2 dir) {
	Vector2 c = RectCenter(plrBody);
	float r = ATTACK_REACH;
	float offX = dir.x * (plrBody.width / 2 + r / 2 - 10);
	float offY = dir.y * (plrBody.height / 2 + r / 2 - 10);
	return (Rectangle){c.x + offX - r / 2, c.y + offY - r / 2, r, r};
}

static void TryMove(Minion* m, Vector2 delta, Vector2 worldSize, LoadedAssetsClass* Assets) {
	float half = m->size / 2;

	Vector2 nx = {m->pos.x + delta.x, m->pos.y};
	nx.x = fminf(fmaxf(nx.x, half), worldSize.x - half);
	if (Assets == NULL || !Assets->CheckColisions(Assets, MinionRect(nx, m->size))) { m->pos.x = nx.x; }
	else { m->wanderDir.x = -m->wanderDir.x; }

	Vector2 ny = {m->pos.x, m->pos.y + delta.y};
	ny.y = fminf(fmaxf(ny.y, half), worldSize.y - half);
	if (Assets == NULL || !Assets->CheckColisions(Assets, MinionRect(ny, m->size))) { m->pos.y = ny.y; }
	else { m->wanderDir.y = -m->wanderDir.y; }
}

static void UpdateSwing(MinionField* f, float dt, MinionInput in) {
	if (f->attackCd > 0) { f->attackCd -= dt; }
	if (f->attackT > 0)  { f->attackT -= dt; }

	if (in.attackPressed && in.plrAlive && f->attackCd <= 0) {
		f->attackT = ATTACK_DURATION;
		f->attackCd = ATTACK_COOLDOWN;
		f->attackDir = (in.facing.x == 0 && in.facing.y == 0) ? (Vector2){0, 1} : Norm(in.facing);
		f->swingId++;
	}
	if (f->attackT > 0) { f->attackRect = BuildAttackRect(in.plrBody, f->attackDir); }
}

static Vector2 ChooseDirection(Minion* m, float dt, Vector2 plrC, bool plrAlive) {
	Vector2 toPlr = {plrC.x - m->pos.x, plrC.y - m->pos.y};
	float dist = sqrtf(toPlr.x * toPlr.x + toPlr.y * toPlr.y);
	bool chase = plrAlive && dist < AGGRO_RANGE;

	if (chase && !m->chasing) { m->alertT = ALERT_TIME; }
	m->chasing = chase;
	if (chase) { return Norm(toPlr); }

	m->wanderT -= dt;
	if (m->wanderT <= 0) {
		m->wanderT = 1.0f + Rand01() * 2.0f;
		float a = Rand01() * 6.2831f;
		m->wanderDir = (Rand01() < 0.3f) ? (Vector2){0, 0} : (Vector2){cosf(a), sinf(a)};
	}
	return m->wanderDir;
}

static void ApplySwingHit(MinionField* f, Minion* m, Vector2 plrC, Score* score) {
	const MinionStats* st = &STATS[m->kind];
	if (st->boss || f->attackT <= 0 || m->lastSwingHit == f->swingId) { return; }
	if (!CheckCollisionRecs(f->attackRect, MinionRect(m->pos, m->size))) { return; }

	m->lastSwingHit = f->swingId;
	m->hp--;
	m->hitFlash = 0.18f;
	Vector2 kb = Norm((Vector2){m->pos.x - plrC.x, m->pos.y - plrC.y});
	float force = (m->kind == MINION_TANK) ? 5.0f : 10.0f;
	m->knock = (Vector2){kb.x * force, kb.y * force};

	if (m->hp > 0) { return; }

	m->alive = false;
	m->chasing = false;
	m->deathT = 0.3f;
	m->respawnT = RESPAWN_TIME + Rand01() * 2.0f;
	if (score != NULL) { Score_AddKill(score, st->points, (Vector2){m->pos.x, m->pos.y - m->size}); }
}

static void UpdateDead(Minion* m, float dt, Vector2 worldSize, Rectangle plrBody, LoadedAssetsClass* Assets) {
	if (m->deathT > 0) { m->deathT -= dt; }
	if (m->permanent) { return; }
	m->respawnT -= dt;
	if (m->respawnT <= 0) { ReviveMinion(m, worldSize, plrBody, Assets); }
}

int Minions_Update(MinionField* f, float dt, MinionInput in, Score* score, Vector2 worldSize, LoadedAssetsClass* Assets) {
	Vector2 plrC = RectCenter(in.plrBody);
	int touched = -1;

	UpdateSwing(f, dt, in);

	for (int i = 0; i < f->count; i++) {
		Minion* m = &f->m[i];

		if (!m->alive) {
			UpdateDead(m, dt, worldSize, in.plrBody, Assets);
			continue;
		}

		const MinionStats* st = &STATS[m->kind];
		m->bob += dt * (m->kind == MINION_BAT ? 14.0f : 6.0f);
		if (m->hitFlash > 0) { m->hitFlash -= dt; }
		if (m->alertT > 0) { m->alertT -= dt; }

		Vector2 dir = ChooseDirection(m, dt, plrC, in.plrAlive);
		float speed = st->speed * (m->hitFlash > 0 ? 0.3f : 1.0f);
		Vector2 delta = {dir.x * speed * dt + m->knock.x, dir.y * speed * dt + m->knock.y};
		TryMove(m, delta, worldSize, Assets);
		m->knock.x *= 0.82f;
		m->knock.y *= 0.82f;

		ApplySwingHit(f, m, plrC, score);
		if (!m->alive) { continue; }

		bool touching = CheckCollisionRecs(MinionRect(m->pos, m->size), in.plrBody);
		if (touched < 0 && in.allowEncounter && touching) { touched = i; }
	}

	return touched;
}

static void DrawMinionBody(const Minion* m) {
	Color c = (m->hitFlash > 0) ? WHITE : STATS[m->kind].color;

	switch (m->kind) {
		case MINION_SLIME: Sprite_DrawSlime(m->pos, m->size, m->bob, c, 1.0f); break;
		case MINION_BAT:   Sprite_DrawBat(m->pos, m->size, m->bob, c, 1.0f);   break;
		case MINION_TANK:  Sprite_DrawTank(m->pos, m->size, m->bob, c, 1.0f);  break;
		case MINION_BOSS: {
			float pulse = 0.5f + 0.5f * sinf(m->bob * 0.8f);
			DrawCircleV(m->pos, m->size * 0.85f, Fade(RED, 0.12f + 0.1f * pulse));
			Vector2 c2 = {m->pos.x, m->pos.y + sinf(m->bob) * 2};
			Sprite_DrawDemon(c2, m->size, Sprite_DemonPalette(), 1.0f);
			break;
		}
		default: break;
	}
}

static void DrawAlert(const Minion* m) {
	if (m->alertT <= 0) { return; }
	float k = m->alertT / ALERT_TIME;
	int fs = 34;
	int x = (int)m->pos.x - MeasureText("!", fs) / 2;
	int y = (int)(m->pos.y - m->size / 2 - 44 - (1.0f - k) * 6);
	DrawText("!", x + 2, y + 2, fs, Fade(BLACK, k));
	DrawText("!", x, y, fs, Fade(RED, k));
}

static void DrawDeath(const Minion* m) {
	if (m->deathT <= 0) { return; }
	float k = 1.0f - m->deathT / 0.3f;
	DrawCircleLines((int)m->pos.x, (int)m->pos.y, m->size * (0.5f + k), Fade(WHITE, 1.0f - k));
	DrawCircle((int)m->pos.x, (int)m->pos.y, m->size * 0.4f * (1.0f - k), Fade(STATS[m->kind].color, 1.0f - k));
}

void Minions_Draw(const MinionField* f) {
	for (int i = 0; i < f->count; i++) {
		const Minion* m = &f->m[i];

		if (!m->alive) {
			DrawDeath(m);
			continue;
		}

		DrawMinionBody(m);
		DrawAlert(m);

		if (m->hp < m->maxHp) {
			float w = m->size;
			int bx = (int)(m->pos.x - w / 2), by = (int)(m->pos.y - m->size / 2 - 12);
			DrawRectangle(bx, by, (int)w, 5, Fade(BLACK, 0.6f));
			DrawRectangle(bx, by, (int)(w * m->hp / m->maxHp), 5, RED);
		}
	}

	if (f->attackT > 0) {
		float k = 1.0f - f->attackT / ATTACK_DURATION;
		Vector2 c = RectCenter(f->attackRect);
		float base = atan2f(f->attackDir.y, f->attackDir.x) * RAD2DEG;
		float start = base - 70 + k * 40;
		DrawRing(c, ATTACK_REACH * 0.25f, ATTACK_REACH * 0.5f, start, start + 100, 12, Fade(WHITE, 0.85f * (1.0f - k)));
	}
}
