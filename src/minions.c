#include <minions.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define ATTACK_DURATION   0.15f
#define ATTACK_COOLDOWN   0.30f
#define ATTACK_REACH      95.0f
#define CONTACT_DAMAGE    8
#define CONTACT_COOLDOWN  0.8f
#define RESPAWN_TIME      4.0f
#define AGGRO_RANGE       330.0f
#define MIN_SPAWN_DIST    300.0f

typedef struct {
	float size;
	int hp;
	float speed;
	int points;
	Color color;
} MinionStats;

static const MinionStats STATS[MINION_KIND_COUNT] = {
	[MINION_SLIME] = {34, 2,  70, 10, {90, 200, 90, 255}},
	[MINION_BAT]   = {26, 1, 140, 15, {150, 80, 200, 255}},
	[MINION_TANK]  = {44, 4,  50, 30, {230, 130, 40, 255}},
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

static void ReviveMinion(Minion* m, Vector2 worldSize, Rectangle plrBody, LoadedAssetsClass* Assets) {
	MinionKind kind = RandomKind();
	const MinionStats* st = &STATS[kind];
	Vector2 pos;
	if (!FindFreeSpot(&pos, st->size, worldSize, plrBody, Assets)) {
		m->respawnT = 1.0f;
		return;
	}

	memset(m, 0, sizeof(*m));
	m->alive = true;
	m->kind = kind;
	m->pos = pos;
	m->size = st->size;
	m->hp = st->hp;
	m->maxHp = st->hp;
	m->bob = Rand01() * 6.28f;
	m->lastSwingHit = -1;
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

void Minions_Update(MinionField* f, float dt, Rectangle plrBody, Vector2 facing, bool attackPressed,
                    int* plrHp, Score* score, Vector2 worldSize, LoadedAssetsClass* Assets) {
	bool plrAlive = *plrHp > 0;
	Vector2 plrC = RectCenter(plrBody);

	if (f->attackCd > 0)   { f->attackCd -= dt; }
	if (f->attackT > 0)    { f->attackT -= dt; }
	if (f->contactCd > 0)  { f->contactCd -= dt; }
	if (f->hurtFlash > 0)  { f->hurtFlash -= dt; }

	if (attackPressed && plrAlive && f->attackCd <= 0) {
		f->attackT = ATTACK_DURATION;
		f->attackCd = ATTACK_COOLDOWN;
		f->attackDir = (facing.x == 0 && facing.y == 0) ? (Vector2){0, 1} : Norm(facing);
		f->swingId++;
	}
	if (f->attackT > 0) { f->attackRect = BuildAttackRect(plrBody, f->attackDir); }

	for (int i = 0; i < f->count; i++) {
		Minion* m = &f->m[i];

		if (!m->alive) {
			if (m->deathT > 0) { m->deathT -= dt; }
			m->respawnT -= dt;
			if (m->respawnT <= 0) { ReviveMinion(m, worldSize, plrBody, Assets); }
			continue;
		}

		const MinionStats* st = &STATS[m->kind];
		m->bob += dt * (m->kind == MINION_BAT ? 14.0f : 6.0f);
		if (m->hitFlash > 0) { m->hitFlash -= dt; }

		Vector2 toPlr = {plrC.x - m->pos.x, plrC.y - m->pos.y};
		float dist = sqrtf(toPlr.x * toPlr.x + toPlr.y * toPlr.y);
		Vector2 dir;
		if (plrAlive && dist < AGGRO_RANGE) {
			dir = Norm(toPlr);
		} else {
			m->wanderT -= dt;
			if (m->wanderT <= 0) {
				m->wanderT = 1.0f + Rand01() * 2.0f;
				float a = Rand01() * 6.2831f;
				m->wanderDir = (Rand01() < 0.3f) ? (Vector2){0, 0} : (Vector2){cosf(a), sinf(a)};
			}
			dir = m->wanderDir;
		}

		float speed = st->speed * (m->hitFlash > 0 ? 0.3f : 1.0f);
		Vector2 delta = {dir.x * speed * dt + m->knock.x, dir.y * speed * dt + m->knock.y};
		TryMove(m, delta, worldSize, Assets);
		m->knock.x *= 0.82f;
		m->knock.y *= 0.82f;

		Rectangle body = MinionRect(m->pos, m->size);

		if (f->attackT > 0 && m->lastSwingHit != f->swingId && CheckCollisionRecs(f->attackRect, body)) {
			m->lastSwingHit = f->swingId;
			m->hp--;
			m->hitFlash = 0.18f;
			Vector2 kb = Norm((Vector2){m->pos.x - plrC.x, m->pos.y - plrC.y});
			float force = (m->kind == MINION_TANK) ? 5.0f : 10.0f;
			m->knock = (Vector2){kb.x * force, kb.y * force};

			if (m->hp <= 0) {
				m->alive = false;
				m->deathT = 0.3f;
				m->respawnT = RESPAWN_TIME + Rand01() * 2.0f;
				if (score != NULL) { Score_AddKill(score, st->points, (Vector2){m->pos.x, m->pos.y - m->size}); }
				continue;
			}
		}

		if (plrAlive && f->contactCd <= 0 && CheckCollisionRecs(body, plrBody)) {
			*plrHp -= CONTACT_DAMAGE;
			if (*plrHp < 0) { *plrHp = 0; }
			f->contactCd = CONTACT_COOLDOWN;
			f->hurtFlash = 0.25f;
			Vector2 kb = Norm((Vector2){m->pos.x - plrC.x, m->pos.y - plrC.y});
			m->knock = (Vector2){kb.x * 8, kb.y * 8};
		}
	}
}

static void DrawSlime(Vector2 p, float s, float bob, Color c) {
	float squish = sinf(bob) * 0.12f;
	float w = s * (1.0f + squish), h = s * 0.8f * (1.0f - squish);
	DrawEllipse((int)p.x, (int)(p.y + s / 2 - 2), s * 0.5f, s * 0.15f, Fade(BLACK, 0.3f));
	DrawEllipse((int)p.x, (int)(p.y + s / 2 - h / 2), w / 2, h / 2, c);
	DrawCircle((int)(p.x - s * 0.18f), (int)(p.y + s / 2 - h * 0.6f), s * 0.08f, BLACK);
	DrawCircle((int)(p.x + s * 0.18f), (int)(p.y + s / 2 - h * 0.6f), s * 0.08f, BLACK);
}

static void DrawBat(Vector2 p, float s, float bob, Color c) {
	float flap = sinf(bob) * s * 0.35f;
	float y = p.y + sinf(bob * 0.3f) * 4;
	DrawEllipse((int)p.x, (int)(p.y + s * 0.8f), s * 0.35f, s * 0.1f, Fade(BLACK, 0.25f));
	DrawTriangle((Vector2){p.x - s * 0.2f, y}, (Vector2){p.x - s * 0.8f, y - flap}, (Vector2){p.x - s * 0.25f, y + s * 0.2f}, c);
	DrawTriangle((Vector2){p.x + s * 0.2f, y}, (Vector2){p.x + s * 0.25f, y + s * 0.2f}, (Vector2){p.x + s * 0.8f, y - flap}, c);
	DrawCircle((int)p.x, (int)y, s * 0.32f, c);
	DrawCircle((int)(p.x - s * 0.12f), (int)(y - s * 0.05f), s * 0.07f, YELLOW);
	DrawCircle((int)(p.x + s * 0.12f), (int)(y - s * 0.05f), s * 0.07f, YELLOW);
}

static void DrawTank(Vector2 p, float s, float bob, Color c) {
	float step = fabsf(sinf(bob)) * 3;
	Rectangle r = {p.x - s / 2, p.y - s / 2 - step, s, s};
	DrawEllipse((int)p.x, (int)(p.y + s / 2), s * 0.55f, s * 0.15f, Fade(BLACK, 0.3f));
	DrawRectangleRounded(r, 0.35f, 6, c);
	DrawRectangleRoundedLinesEx(r, 0.35f, 6, 2, Fade(BLACK, 0.5f));
	DrawRectangle((int)(p.x - s * 0.28f), (int)(p.y - s * 0.15f - step), (int)(s * 0.16f), (int)(s * 0.08f), BLACK);
	DrawRectangle((int)(p.x + s * 0.12f), (int)(p.y - s * 0.15f - step), (int)(s * 0.16f), (int)(s * 0.08f), BLACK);
}

void Minions_Draw(const MinionField* f) {
	for (int i = 0; i < f->count; i++) {
		const Minion* m = &f->m[i];

		if (!m->alive) {
			if (m->deathT > 0) {
				float k = 1.0f - m->deathT / 0.3f;
				DrawCircleLines((int)m->pos.x, (int)m->pos.y, m->size * (0.5f + k), Fade(WHITE, 1.0f - k));
				DrawCircle((int)m->pos.x, (int)m->pos.y, m->size * 0.4f * (1.0f - k), Fade(STATS[m->kind].color, 1.0f - k));
			}
			continue;
		}

		Color c = (m->hitFlash > 0) ? WHITE : STATS[m->kind].color;
		switch (m->kind) {
			case MINION_SLIME: DrawSlime(m->pos, m->size, m->bob, c); break;
			case MINION_BAT:   DrawBat(m->pos, m->size, m->bob, c);   break;
			case MINION_TANK:  DrawTank(m->pos, m->size, m->bob, c);  break;
			default: break;
		}

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

void Minions_DrawHurtOverlay(const MinionField* f) {
	if (f->hurtFlash > 0) {
		DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(RED, 0.3f * f->hurtFlash / 0.25f));
	}
}

bool Minions_PlayerBlink(const MinionField* f) {
	return f->contactCd > 0 && ((int)(f->contactCd * 15) % 2) == 0;
}
