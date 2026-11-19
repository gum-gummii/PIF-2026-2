#include <battle.h>
#include <encounter.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define TEXT_CPS          42.0f
#define TP_PACIFY_COST    32.0f
#define SOUL_SPEED        230.0f
#define SOUL_HIT_R        5.0f
#define GRAZE_R           16.0f
#define INVULN_TIME       1.0f
#define ATTACK_TIME       1.15f
#define SPAWN_STOP_EARLY  0.9f
#define DIALOGUE_TIME     2.2f
#define MENU_LOCK_START   0.3f
#define PACIFY_INDEX      (ACT_COUNT - 1)

static const char* BUTTON_LABELS[BUTTON_COUNT] = {"LUTAR", "AGIR", "ITEM", "POUPAR", "DEFENDER"};
static const char* SPARE_LABELS[2] = {"Poupar", "Fugir"};
#define PATTERN_TYPES     9
static const BulletPattern ALL_PATTERNS[PATTERN_TYPES] = {
	PATTERN_RAIN, PATTERN_SIDES, PATTERN_FAN,
	PATTERN_HEARTS, PATTERN_SPADES, PATTERN_CLUBS, PATTERN_DIAMONDS, PATTERN_PAW, PATTERN_WALL,
};

#define HAZARD_STOP_EARLY 1.9f

#define HEART_FUSE        1.1f  // atk1
#define HEART_BULLETS     18

#define SPADE_APPEAR      0.6f  // atk2
#define SPADE_OPEN        0.15f
#define SPADE_FIRE        0.5f
#define SPADE_CLOSE       0.25f
#define LASER_W           26.0f

#define CLUB_APPEAR       0.4f  // atk3
#define CLUB_GROW         0.3f
#define CLUB_HOLD         0.25f
#define CLUB_ROTATE       0.3f
#define CLUB_BLAST        0.1f
#define CLUB_FADE         0.4f
#define ROD_LEN           62.0f
#define ROD_W             9.0f

#define DIAMOND_FUSE      0.75f // atk4
#define DIAMOND_SPEED     430.0f

#define PAW_WARN          0.95f // atk5
#define PAW_SLAM          0.12f
#define PAW_HOLD          0.45f
#define PAW_RETREAT       0.2f

#define WALL_BULLETS      56    // atk6
#define WALL_GAP          72.0f

static bool Confirm(void) { return IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_ENTER); }
static bool Cancel(void) { return IsKeyPressed(KEY_X) || IsKeyPressed(KEY_BACKSPACE); }
static bool NavLeft(void) { return IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A); }
static bool NavRight(void) { return IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D); }
static bool NavUp(void) { return IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W); }
static bool NavDown(void) { return IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S); }

static float Clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

static Vector2 Norm(Vector2 v) {
	float len = sqrtf(v.x * v.x + v.y * v.y);
	if (len < 0.0001f) { return (Vector2){0, 0}; }
	return (Vector2){v.x / len, v.y / len};
}

static Rectangle IdleBox(void) {
	return (Rectangle){80, (float)GetScreenHeight() - 348, (float)GetScreenWidth() - 160, 190};
}

static Rectangle BulletBox(void) {
	return (Rectangle){GetScreenWidth() / 2.0f - 190, (float)GetScreenHeight() - 348, 380, 190};
}

static Vector2 EnemyCenter(void) {
	return (Vector2){GetScreenWidth() * 0.66f + 54, 204};
}

static float EnemySize(const Battle* b) { return b->def->size; }
static float EnemyTop(const Battle* b) { return EnemyCenter().y - EnemySize(b) / 2; }
static float EnemyLeft(const Battle* b) { return EnemyCenter().x - EnemySize(b) / 2; }

static Rectangle ButtonRect(int i) {
	float w = 170, h = 44, gap = 14;
	float total = BUTTON_COUNT * w + (BUTTON_COUNT - 1) * gap;
	return (Rectangle){(GetScreenWidth() - total) / 2 + i * (w + gap), (float)GetScreenHeight() - 136, w, h};
}

static Rectangle PanelRect(void) {
	return (Rectangle){(GetScreenWidth() - 480) / 2.0f, (float)GetScreenHeight() - 88, 480, 88};
}

static bool EnemyAngry(const Battle* b) { return b->enemy.hp < b->enemy.maxHp / 2; }
static bool EnemySpareable(const Battle* b) { return b->enemy.mercy >= 100.0f; }

static const char* ActLabel(const Battle* b, int i) {
	if (i == 0) { return "Checar"; }
	if (i == PACIFY_INDEX) { return "Pacificar"; }
	return b->def->acts[i - 1].label;
}

static int ItemTypes(const Battle* b) {
	int n = 0;
	for (int i = 0; i < ITEM_SLOTS; i++) { if (b->items[i].count > 0) { n++; } }
	return n;
}

static int ItemSlot(const Battle* b, int n) {
	int seen = 0;
	for (int i = 0; i < ITEM_SLOTS; i++) {
		if (b->items[i].count <= 0) { continue; }
		if (seen == n) { return i; }
		seen++;
	}
	return -1;
}

static int ListCount(const Battle* b) {
	if (b->state == B_ACT_LIST) { return ACT_COUNT; }
	if (b->state == B_ITEM_LIST) { return ItemTypes(b); }
	if (b->state == B_SPARE_LIST) { return 2; }
	return 1;
}

static int ScaledDamage(const Battle* b, int base) {
	int dmg = (int)ceilf(base * b->def->damageMult);
	return dmg < 1 ? 1 : dmg;
}

static void AddShapedBullet(Battle* b, Vector2 pos, Vector2 vel, float r, int dmg, BulletShape shape) {
	for (int i = 0; i < MAX_BULLETS; i++) {
		if (b->bullets[i].active) { continue; }
		b->bullets[i] = (Bullet){true, pos, vel, r, dmg, false, shape};
		return;
	}
}

static void AddBullet(Battle* b, Vector2 pos, Vector2 vel, float r, int dmg) {
	AddShapedBullet(b, pos, vel, r, dmg, BULLET_ROUND);
}

static void AddHazard(Battle* b, Hazard h) {
	for (int i = 0; i < MAX_HAZARDS; i++) {
		if (b->hazards[i].kind != HZ_NONE) { continue; }
		b->hazards[i] = h;
		return;
	}
}

static bool IsHazardPattern(BulletPattern p) {
	return p == PATTERN_HEARTS || p == PATTERN_SPADES || p == PATTERN_CLUBS || p == PATTERN_DIAMONDS || p == PATTERN_PAW;
}

static BulletPattern PickPattern(const Battle* b) {
	BulletPattern options[PATTERN_TYPES];
	int n = 0;
	for (int i = 0; i < PATTERN_TYPES; i++) {
		if (b->def->patterns & ALL_PATTERNS[i]) { options[n++] = ALL_PATTERNS[i]; }
	}
	if (n == 0) { return PATTERN_RAIN; }
	if (n == 1) { return options[0]; }

	BulletPattern p = options[GetRandomValue(0, n - 1)];
	while (p == b->pattern) { p = options[GetRandomValue(0, n - 1)]; }
	return p;
}

static void PickFlavor(Battle* b) {
	const EnemyDef* d = b->def;
	const char* msg;
	if (b->turn == 0) { msg = d->firstFlavor; }
	else if (EnemySpareable(b)) { msg = d->spareFlavor; }
	else if (EnemyAngry(b)) { msg = d->angryFlavor; }
	else { msg = d->flavor[b->turn % 3]; }
	snprintf(b->flavor, sizeof(b->flavor), "%s", msg);
}

static void SetText(Battle* b, const char* msg, BattleState next) {
	snprintf(b->text, sizeof(b->text), "%s", msg);
	b->textT = 0;
	b->afterText = next;
	b->state = B_TEXT;
	b->stateT = 0;
}

static void GoTo(Battle* b, BattleState s) {
	b->state = s;
	b->stateT = 0;
	b->listIndex = 0;

	switch (s) {
	case B_MENU:
		PickFlavor(b);
		break;
	case B_ATTACK:
		b->attackP = 0;
		b->attackAcc = 0;
		b->attackMissed = false;
		b->dmgApplied = false;
		break;
	case B_ENEMY_DIALOGUE: {
		const EnemyDef* d = b->def;
		const char* const* set = EnemyAngry(b) ? d->linesAngry : (b->enemy.mercy >= 50.0f ? d->linesCalm : d->linesNormal);
		snprintf(b->line, sizeof(b->line), "%s", set[GetRandomValue(0, 2)]);
		break;
	}
	case B_ENEMY_ATTACK: {
		Rectangle box = BulletBox();
		b->soul = (Vector2){box.x + box.width / 2, box.y + box.height / 2};
		memset(b->bullets, 0, sizeof(b->bullets));
		memset(b->hazards, 0, sizeof(b->hazards));
		b->lastSide = -1;
		b->pattern = PickPattern(b);
		b->spawnT = 0.3f;
		b->invuln = 0;
		break;
	}
	case B_LOST:
		b->outcome = BATTLE_LOST;
		for (int i = 0; i < MAX_SHARDS; i++) {
			b->shards[i] = b->soul;
			float ang = (float)GetRandomValue(0, 628) / 100.0f;
			float sp = (float)GetRandomValue(60, 220);
			b->shardVel[i] = (Vector2){cosf(ang) * sp, sinf(ang) * sp - 120};
		}
		break;
	case B_DONE:
		b->active = false;
		break;
	default:
		break;
	}
}

static void Finish(Battle* b, BattleOutcome outcome, const char* msg) {
	b->outcome = outcome;
	b->outcomeT = 0;
	SetText(b, msg, B_DONE);
}

void Battle_Start(Battle* b, const EnemyDef* def, Texture2D plrTexture, Rectangle plrSrc) {
	memset(b, 0, sizeof(Battle));
	b->active = true;
	b->def = def;
	b->plrTexture = plrTexture;
	b->plrSrc = plrSrc;
	b->enemy.hp = b->enemy.maxHp = def->maxHp;
	b->items[0] = (BattleItem){"Bolo", 40, 2};
	b->items[1] = (BattleItem){"Suco", 20, 3};
	b->items[2] = (BattleItem){"Biscoito", 10, 4};
	b->box = IdleBox();
	b->menuLock = MENU_LOCK_START;
	b->state = B_FADE_IN;
	b->stateT = 0;
}

void Battle_Stop(Battle* b) { b->active = false; }
bool Battle_IsActive(const Battle* b) { return b->active; }
BattleOutcome Battle_Outcome(const Battle* b) { return b->outcome; }
const EnemyDef* Battle_Enemy(const Battle* b) { return b->def; }

bool Battle_ConsumeRestart(Battle* b) {
	bool r = b->restartRequested;
	b->restartRequested = false;
	return r;
}

static void SoulMove(Battle* b, float dt) {
	float dx = 0, dy = 0;
	if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) { dx -= 1; }
	if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) { dx += 1; }
	if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) { dy -= 1; }
	if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) { dy += 1; }
	Vector2 d = Norm((Vector2){dx, dy});
	b->soul.x += d.x * SOUL_SPEED * dt;
	b->soul.y += d.y * SOUL_SPEED * dt;

	Rectangle box = BulletBox();
	b->soul.x = Clampf(b->soul.x, box.x + 5 + 14, box.x + box.width - 5 - 14);
	b->soul.y = Clampf(b->soul.y, box.y + 5 + 12, box.y + box.height - 5 - 12);
}

#define SPADE_END  (SPADE_APPEAR + SPADE_OPEN + SPADE_FIRE + SPADE_CLOSE)
#define CLUB_HIT_T (CLUB_APPEAR + CLUB_GROW + CLUB_HOLD + CLUB_ROTATE)
#define CLUB_END   (CLUB_HIT_T + CLUB_BLAST + CLUB_FADE)
#define PAW_END    (PAW_WARN + PAW_SLAM + PAW_HOLD + PAW_RETREAT)

static float PatternSpeed(const Battle* b) {
	return b->def->bulletSpeed * (EnemyAngry(b) ? 1.25f : 1.0f);
}

static float RandF(float lo, float hi) {
	return lo + (hi - lo) * (float)GetRandomValue(0, 10000) / 10000.0f;
}

static float Dist(Vector2 a, Vector2 c) {
	float dx = a.x - c.x, dy = a.y - c.y;
	return sqrtf(dx * dx + dy * dy);
}

static float SegDist(Vector2 p, Vector2 a, Vector2 c) {
	float vx = c.x - a.x, vy = c.y - a.y;
	float len2 = vx * vx + vy * vy;
	float t = len2 > 0 ? Clampf(((p.x - a.x) * vx + (p.y - a.y) * vy) / len2, 0, 1) : 0;
	return Dist(p, (Vector2){a.x + vx * t, a.y + vy * t});
}

static Vector2 RandomInBox(float margin) {
	Rectangle box = BulletBox();
	return (Vector2){RandF(box.x + margin, box.x + box.width - margin), RandF(box.y + margin, box.y + box.height - margin)};
}

static Vector2 RandomAwayFromSoul(const Battle* b, float margin, float minDist) {
	Vector2 p = RandomInBox(margin);
	for (int i = 0; i < 16 && Dist(p, b->soul) < minDist; i++) { p = RandomInBox(margin); }
	return p;
}

static void HurtSoul(Battle* b, int dmg, int* plrHp) {
	if (b->invuln > 0) { return; }
	if (b->defending) { dmg = (dmg + 1) / 2; }
	*plrHp -= dmg;
	if (*plrHp < 0) { *plrHp = 0; }
	b->invuln = INVULN_TIME;
	b->hurtFlash = 0.25f;
}

static Vector2 HeartPoint(float t) {
	float s = sinf(t);
	return (Vector2){
		16.0f * s * s * s / 17.0f,
		-(13.0f * cosf(t) - 5.0f * cosf(2 * t) - 2.0f * cosf(3 * t) - cosf(4 * t)) / 17.0f + 0.15f,
	};
}

// atk2
static float SpadeLaserWidth(const Hazard* h, bool* hurts) {
	float t0 = SPADE_APPEAR, t1 = t0 + SPADE_OPEN, t2 = t1 + SPADE_FIRE;
	*hurts = false;
	if (h->t < t0) { return 0; }
	if (h->t < t1) { return LASER_W * (h->t - t0) / SPADE_OPEN; }
	if (h->t < t2) { *hurts = true; return LASER_W; }
	if (h->t < SPADE_END) { return LASER_W * (1.0f - (h->t - t2) / SPADE_CLOSE); }
	return 0;
}

static Vector2 SpadeLaserEnd(const Hazard* h) {
	return (Vector2){h->pos.x + cosf(h->dir) * h->len, h->pos.y + sinf(h->dir) * h->len};
}

// atk3
typedef struct {
	Vector2 from[3];
	Vector2 to[3];
	int count;
	float width;
	float alpha;
} ClubRods;

static ClubRods ClubGeometry(const Hazard* h) {
	ClubRods r = {0};
	r.width = ROD_W;
	r.alpha = 1.0f;

	float tGrow = CLUB_APPEAR;
	float tRot = tGrow + CLUB_GROW + CLUB_HOLD;
	float tFade = CLUB_HIT_T + CLUB_BLAST;
	Vector2 o = {h->pos.x + cosf(h->dir) * 12, h->pos.y + sinf(h->dir) * 12};

	if (h->t < tGrow) { return r; }

	if (h->t < CLUB_HIT_T) {
		float len = ROD_LEN * Clampf((h->t - tGrow) / CLUB_GROW, 0, 1);
		float k = Clampf((h->t - tRot) / CLUB_ROTATE, 0, 1);
		float side = (PI / 4) * (1.0f - k * k);
		float angs[3] = {h->dir, h->dir - side, h->dir + side};
		for (int i = 0; i < 3; i++) {
			r.from[i] = o;
			r.to[i] = (Vector2){o.x + cosf(angs[i]) * len, o.y + sinf(angs[i]) * len};
		}
		r.count = 3;
		return r;
	}

	float e = Clampf((h->t - CLUB_HIT_T) / CLUB_BLAST, 0, 1);
	float len = ROD_LEN * (1.0f + 2.0f * e);
	r.width = ROD_W * (1.0f + 0.8f * e);
	r.alpha = 1.0f - Clampf((h->t - tFade) / CLUB_FADE, 0, 1);
	r.from[0] = o;
	r.to[0] = (Vector2){o.x + cosf(h->dir) * len, o.y + sinf(h->dir) * len};
	r.count = 1;
	return r;
}

// atk5
static float PawSlide(const Hazard* h, bool* hurts) {
	float t0 = PAW_WARN, t1 = t0 + PAW_SLAM, t2 = t1 + PAW_HOLD;
	*hurts = false;
	if (h->t < t0) { return 0; }
	if (h->t < t1) {
		float k = (h->t - t0) / PAW_SLAM;
		*hurts = k > 0.85f;
		return k;
	}
	if (h->t < t2) { *hurts = true; return 1; }
	if (h->t < PAW_END) { return 1.0f - (h->t - t2) / PAW_RETREAT; }
	return 0;
}

static Vector2 PawForward(int side) {
	switch (side) {
	case 0:  return (Vector2){1, 0};
	case 1:  return (Vector2){-1, 0};
	case 2:  return (Vector2){0, 1};
	default: return (Vector2){0, -1};
	}
}

static void ExplodeHeart(Battle* b, const Hazard* h, float spd) {
	for (int k = 0; k < HEART_BULLETS; k++) {
		Vector2 p = HeartPoint(2 * PI * k / HEART_BULLETS);
		Vector2 pos = {h->pos.x + p.x * 10, h->pos.y + p.y * 10};
		AddShapedBullet(b, pos, (Vector2){p.x * 150 * spd, p.y * 150 * spd}, 6, ScaledDamage(b, 8), BULLET_HEART);
	}
}

static void ExplodeDiamond(Battle* b, const Hazard* h, float spd) {
	Vector2 target = b->soul;
	Vector2 offs[4] = {{0, -18}, {14, 0}, {0, 18}, {-14, 0}};
	for (int k = 0; k < 4; k++) {
		Vector2 start = {h->pos.x + offs[k].x, h->pos.y + offs[k].y};
		Vector2 d = Norm((Vector2){target.x - start.x, target.y - start.y});
		if (d.x == 0 && d.y == 0) { d = Norm(offs[k]); }
		AddShapedBullet(b, start, (Vector2){d.x * DIAMOND_SPEED * spd, d.y * DIAMOND_SPEED * spd}, 7, ScaledDamage(b, 9), BULLET_DIAMOND);
	}
}

static void UpdateHazards(Battle* b, float dt, int* plrHp) {
	float spd = PatternSpeed(b);

	for (int i = 0; i < MAX_HAZARDS; i++) {
		Hazard* h = &b->hazards[i];
		if (h->kind == HZ_NONE) { continue; }
		h->t += dt;

		switch (h->kind) {
		case HZ_HEART:
			if (h->t >= HEART_FUSE) {
				ExplodeHeart(b, h, spd);
				h->kind = HZ_NONE;
			}
			break;

		case HZ_SPADE: {
			bool hurts;
			SpadeLaserWidth(h, &hurts);
			if (hurts && SegDist(b->soul, h->pos, SpadeLaserEnd(h)) < LASER_W * 0.5f + SOUL_HIT_R) {
				HurtSoul(b, ScaledDamage(b, 10), plrHp);
			}
			if (h->t >= SPADE_END) { h->kind = HZ_NONE; }
			break;
		}

		case HZ_CLUB: {
			ClubRods r = ClubGeometry(h);
			if (r.alpha > 0.35f) {
				for (int k = 0; k < r.count; k++) {
					if (SegDist(b->soul, r.from[k], r.to[k]) < r.width * 0.5f + SOUL_HIT_R) {
						HurtSoul(b, ScaledDamage(b, 9), plrHp);
					}
				}
			}
			if (h->t >= CLUB_END) { h->kind = HZ_NONE; }
			break;
		}

		case HZ_DIAMOND:
			if (h->t >= DIAMOND_FUSE) {
				ExplodeDiamond(b, h, spd);
				h->kind = HZ_NONE;
			}
			break;

		case HZ_PAW: {
			bool hurts;
			PawSlide(h, &hurts);
			if (hurts && CheckCollisionCircleRec(b->soul, SOUL_HIT_R, h->area)) {
				HurtSoul(b, ScaledDamage(b, 14), plrHp);
			}
			if (h->t >= PAW_END) { h->kind = HZ_NONE; }
			break;
		}

		default:
			h->kind = HZ_NONE;
			break;
		}
	}
}

static void SpawnBossPattern(Battle* b, float spd) {
	Rectangle box = BulletBox();

	switch (b->pattern) {
	case PATTERN_HEARTS:  // atk1
		AddHazard(b, (Hazard){.kind = HZ_HEART, .pos = RandomAwayFromSoul(b, 28, 50)});
		b->spawnT = 0.7f / spd;
		break;

	case PATTERN_SPADES: {  // atk2
		int side = GetRandomValue(0, 2);
		bool aim = GetRandomValue(0, 1) == 1;
		Hazard h = {.kind = HZ_SPADE, .side = side};
		if (side == 2) {
			float x = aim ? b->soul.x : RandF(box.x + 30, box.x + box.width - 30);
			h.pos = (Vector2){x, box.y - 22};
			h.dir = PI / 2;
			h.len = box.height + 40;
		} else {
			float y = aim ? b->soul.y : RandF(box.y + 25, box.y + box.height - 25);
			h.pos = (Vector2){side == 0 ? box.x - 22 : box.x + box.width + 22, y};
			h.dir = side == 0 ? 0 : PI;
			h.len = box.width + 40;
		}
		AddHazard(b, h);
		b->spawnT = 0.75f / spd;
		break;
	}

	case PATTERN_CLUBS: {  // atk3
		bool fromBottom = GetRandomValue(0, 1) == 1;
		float x = RandF(box.x + 40, box.x + box.width - 40);
		AddHazard(b, (Hazard){
			.kind = HZ_CLUB,
			.pos = (Vector2){x, fromBottom ? box.y + box.height - 26 : box.y + 26},
			.dir = fromBottom ? -PI / 2 : PI / 2,
		});
		b->spawnT = 0.85f / spd;
		break;
	}

	case PATTERN_DIAMONDS:  // atk4
		AddHazard(b, (Hazard){.kind = HZ_DIAMOND, .pos = RandomAwayFromSoul(b, 26, 100)});
		b->spawnT = 0.65f / spd;
		break;

	case PATTERN_PAW: {  // atk5
		int side = GetRandomValue(0, 3);
		while (side == b->lastSide) { side = GetRandomValue(0, 3); }
		b->lastSide = side;

		Rectangle a = box;
		if (side == 0) { a.width /= 2; }
		else if (side == 1) { a.width /= 2; a.x += a.width; }
		else if (side == 2) { a.height /= 2; }
		else { a.height /= 2; a.y += a.height; }

		AddHazard(b, (Hazard){.kind = HZ_PAW, .area = a, .side = side});
		b->spawnT = PAW_END + 0.1f;
		break;
	}

	case PATTERN_WALL: {  // atk6
		Vector2 ec = EnemyCenter();
		Vector2 bc = {box.x + box.width / 2, box.y + box.height / 2};
		Vector2 gp = {RandF(box.x + 40, box.x + box.width - 40), RandF(box.y + 30, box.y + box.height - 30)};
		float base = atan2f(bc.y - ec.y, bc.x - ec.x);
		float gapA = atan2f(gp.y - ec.y, gp.x - ec.x);
		float gapHalf = (WALL_GAP * 0.5f) / Dist(ec, gp);

		for (int i = 0; i < WALL_BULLETS; i++) {
			float a = base - PI / 2 + PI * i / (WALL_BULLETS - 1);
			float diff = atan2f(sinf(a - gapA), cosf(a - gapA));
			if (fabsf(diff) < gapHalf) { continue; }
			AddBullet(b, ec, (Vector2){cosf(a) * 200 * spd, sinf(a) * 200 * spd}, 8, ScaledDamage(b, 8));
		}
		b->spawnT = 1.1f / spd;
		break;
	}

	default:
		b->spawnT = 1.0f;
		break;
	}
}

static void SpawnPattern(Battle* b) {
	Rectangle box = BulletBox();
	float spd = PatternSpeed(b);

	if (b->pattern & (PATTERN_HEARTS | PATTERN_SPADES | PATTERN_CLUBS | PATTERN_DIAMONDS | PATTERN_PAW | PATTERN_WALL)) {
		SpawnBossPattern(b, spd);
		return;
	}

	if (b->pattern == PATTERN_RAIN) {
		float x = (float)GetRandomValue((int)box.x + 12, (int)(box.x + box.width) - 12);
		AddBullet(b, (Vector2){x, box.y - 20}, (Vector2){0, 210 * spd}, 9, ScaledDamage(b, 9));
		b->spawnT = 0.2f / spd;
	} else if (b->pattern == PATTERN_SIDES) {
		bool right = GetRandomValue(0, 1) == 1;
		float y = (float)GetRandomValue((int)box.y + 20, (int)(box.y + box.height) - 20);
		float x = right ? box.x + box.width + 30 : box.x - 30;
		AddBullet(b, (Vector2){x, y}, (Vector2){(right ? -270.0f : 270.0f) * spd, 0}, 10, ScaledDamage(b, 9));
		b->spawnT = 0.6f / spd;
	} else {
		Vector2 ec = EnemyCenter();
		Vector2 aim = Norm((Vector2){b->soul.x - ec.x, b->soul.y - ec.y});
		float base = atan2f(aim.y, aim.x);
		for (int i = -2; i <= 2; i++) {
			float a = base + i * 0.22f;
			AddBullet(b, ec, (Vector2){cosf(a) * 240 * spd, sinf(a) * 240 * spd}, 11, ScaledDamage(b, 12));
		}
		b->spawnT = 1.0f / spd;
	}
}

static void UpdateEnemyTurn(Battle* b, float dt, int* plrHp) {
	SoulMove(b, dt);

	float stopEarly = IsHazardPattern(b->pattern) ? HAZARD_STOP_EARLY : SPAWN_STOP_EARLY;
	if (b->stateT < b->def->turnTime - stopEarly) {
		b->spawnT -= dt;
		if (b->spawnT <= 0) { SpawnPattern(b); }
	}

	UpdateHazards(b, dt, plrHp);

	int sw = GetScreenWidth(), sh = GetScreenHeight();
	for (int i = 0; i < MAX_BULLETS; i++) {
		Bullet* bl = &b->bullets[i];
		if (!bl->active) { continue; }

		bl->pos.x += bl->vel.x * dt;
		bl->pos.y += bl->vel.y * dt;
		if (bl->pos.x < -60 || bl->pos.y < -60 || bl->pos.x > sw + 60 || bl->pos.y > sh + 60) {
			bl->active = false;
			continue;
		}

		float dx = bl->pos.x - b->soul.x, dy = bl->pos.y - b->soul.y;
		float dist = sqrtf(dx * dx + dy * dy);

		if (dist < bl->r + SOUL_HIT_R && b->invuln <= 0) {
			HurtSoul(b, bl->dmg, plrHp);
			bl->active = false;
			continue;
		}

		if (!bl->grazed && dist < bl->r + GRAZE_R) {
			bl->grazed = true;
			b->tp = Clampf(b->tp + 2.5f, 0, 100);
		}
	}

	if (*plrHp <= 0) {
		GoTo(b, B_LOST);
		return;
	}

	if (b->stateT >= b->def->turnTime) {
		memset(b->bullets, 0, sizeof(b->bullets));
		memset(b->hazards, 0, sizeof(b->hazards));
		b->defending = false;
		b->turn++;
		GoTo(b, B_MENU);
	}
}

static void DoAct(Battle* b, int act) {
	b->tp = Clampf(b->tp + 4, 0, 100);

	if (act == 0) {
		SetText(b, b->def->checkText, B_ENEMY_DIALOGUE);
		return;
	}

	if (act == PACIFY_INDEX) {
		b->tp -= TP_PACIFY_COST;
		b->enemy.mercy += 60;
		SetText(b, "* Voce lancou PACIFICAR!\n* O clima ficou mais calmo.", B_ENEMY_DIALOGUE);
	} else {
		const EnemyAct* a = &b->def->acts[act - 1];
		b->enemy.mercy += a->mercy;
		SetText(b, a->text, B_ENEMY_DIALOGUE);
	}
	b->enemy.mercy = Clampf(b->enemy.mercy, 0, 100);
}

static void ChooseSpare(Battle* b) {
	const EnemyDef* d = b->def;
	char msg[160];

	if (b->listIndex == 1) {
		if (d->isBoss) { SetText(b, "* Nao da pra fugir do chefe!", B_ENEMY_DIALOGUE); }
		else { Finish(b, BATTLE_FLED, "* Voce fugiu..."); }
		return;
	}

	if (EnemySpareable(b)) {
		snprintf(msg, sizeof(msg), "* Voce poupou %s %s.\n* Ganhou 0 EXP e %d D$.", d->article, d->name, d->gold);
		Finish(b, BATTLE_SPARED, msg);
	} else {
		snprintf(msg, sizeof(msg), "* Voce tentou poupar %s %s...\n* Mas ainda nao quer parar.", d->article, d->name);
		SetText(b, msg, B_ENEMY_DIALOGUE);
	}
}

static void UseItem(Battle* b, int* plrHp) {
	int slot = ItemSlot(b, b->listIndex);
	if (slot < 0) { return; }

	BattleItem* it = &b->items[slot];
	int before = *plrHp;
	*plrHp = (int)Clampf((float)(*plrHp + it->heal), 0, PLAYER_MAX_HP);
	it->count--;

	char msg[128];
	snprintf(msg, sizeof(msg), "* Voce usou %s.\n* Recuperou %d HP!", it->name, *plrHp - before);
	SetText(b, msg, B_ENEMY_DIALOGUE);
}

static void UpdateMainMenu(Battle* b, bool ok) {
	if (NavLeft()) { b->menuIndex = (b->menuIndex + BUTTON_COUNT - 1) % BUTTON_COUNT; }
	if (NavRight()) { b->menuIndex = (b->menuIndex + 1) % BUTTON_COUNT; }
	if (!ok) { return; }

	switch (b->menuIndex) {
	case 0: GoTo(b, B_FIGHT_SELECT); break;
	case 1: GoTo(b, B_ACT_SELECT); break;
	case 2: if (ItemTypes(b) > 0) { GoTo(b, B_ITEM_LIST); } break;
	case 3: GoTo(b, B_SPARE_LIST); break;
	default:
		b->defending = true;
		b->tp = Clampf(b->tp + 16, 0, 100);
		SetText(b, "* Voce se protegeu.", B_ENEMY_DIALOGUE);
		break;
	}
}

static void UpdateMenu(Battle* b, bool ok, bool back, int* plrHp) {
	if (b->state == B_MENU) {
		UpdateMainMenu(b, ok);
		return;
	}

	if (back) {
		GoTo(b, B_MENU);
		return;
	}

	int count = ListCount(b);
	if (NavUp()) { b->listIndex = (b->listIndex + count - 1) % count; }
	if (NavDown()) { b->listIndex = (b->listIndex + 1) % count; }
	if (!ok) { return; }

	switch (b->state) {
	case B_FIGHT_SELECT: GoTo(b, B_ATTACK); break;
	case B_ACT_SELECT:   GoTo(b, B_ACT_LIST); break;
	case B_ACT_LIST:
		if (b->listIndex == PACIFY_INDEX && b->tp < TP_PACIFY_COST) { return; }
		DoAct(b, b->listIndex);
		break;
	case B_ITEM_LIST:    UseItem(b, plrHp); break;
	case B_SPARE_LIST:   ChooseSpare(b); break;
	default: break;
	}
}

static void UpdateAttack(Battle* b, bool ok) {
	b->attackP = Clampf(b->stateT / ATTACK_TIME, 0, 1);

	if (!ok && b->stateT < ATTACK_TIME) { return; }

	if (ok) {
		float acc = 1.0f - fabsf(b->attackP - 0.5f) / 0.5f;
		b->attackAcc = Clampf(acc, 0, 1);
		b->lastDamage = 6 + (int)(54.0f * b->attackAcc * b->attackAcc);
		b->attackMissed = false;
	} else {
		b->attackAcc = 0;
		b->lastDamage = 0;
		b->attackMissed = true;
	}
	GoTo(b, B_ATTACK_ANIM);
}

static void UpdateAttackAnim(Battle* b) {
	if (!b->dmgApplied && b->stateT >= 0.35f) {
		b->dmgApplied = true;
		if (!b->attackMissed) {
			b->enemy.hp -= b->lastDamage;
			if (b->enemy.hp < 0) { b->enemy.hp = 0; }
			b->enemy.hitFlash = 0.25f;
			b->enemy.shake = 14.0f;
			b->tp = Clampf(b->tp + 4 + 14.0f * b->attackAcc, 0, 100);
		}
		b->dmgNumT = 1.0f;
	}

	if (b->stateT < 1.3f) { return; }

	if (b->enemy.hp <= 0) {
		char msg[96];
		snprintf(msg, sizeof(msg), "* VOCE VENCEU!\n* Ganhou %d EXP e %d D$.", b->def->exp, b->def->gold);
		Finish(b, BATTLE_WON, msg);
	} else {
		GoTo(b, B_ENEMY_DIALOGUE);
	}
}

static void UpdateBox(Battle* b, float dt) {
	bool toBullet = b->state == B_ENEMY_DIALOGUE || b->state == B_ENEMY_ATTACK;
	Rectangle target = toBullet ? BulletBox() : IdleBox();
	float k = 1.0f - expf(-14.0f * dt);
	b->box.x += (target.x - b->box.x) * k;
	b->box.y += (target.y - b->box.y) * k;
	b->box.width += (target.width - b->box.width) * k;
	b->box.height += (target.height - b->box.height) * k;
}

static void UpdateTimers(Battle* b, float dt) {
	b->stateT += dt;
	b->time += dt;
	if (b->menuLock > 0) { b->menuLock -= dt; }
	if (b->enemy.hitFlash > 0) { b->enemy.hitFlash -= dt; }
	if (b->enemy.shake > 0) { b->enemy.shake = fmaxf(0, b->enemy.shake - 40.0f * dt); }
	if (b->dmgNumT > 0) { b->dmgNumT -= dt; }
	if (b->invuln > 0) { b->invuln -= dt; }
	if (b->hurtFlash > 0) { b->hurtFlash -= dt; }
	if (b->outcome != BATTLE_ONGOING) { b->outcomeT += dt; }
}

void Battle_Update(Battle* b, float dt, int* plrHp) {
	if (!b->active) { return; }

	UpdateTimers(b, dt);
	UpdateBox(b, dt);

	bool ok = b->menuLock <= 0 && Confirm();
	bool back = b->menuLock <= 0 && Cancel();

	switch (b->state) {
	case B_FADE_IN:
		if (b->stateT >= 0.5f) { GoTo(b, B_MENU); }
		break;
	case B_MENU:
	case B_FIGHT_SELECT:
	case B_ACT_SELECT:
	case B_ACT_LIST:
	case B_ITEM_LIST:
	case B_SPARE_LIST:
		UpdateMenu(b, ok, back, plrHp);
		break;
	case B_ATTACK:
		UpdateAttack(b, ok);
		break;
	case B_ATTACK_ANIM:
		UpdateAttackAnim(b);
		break;
	case B_TEXT:
		b->textT += dt;
		if (ok) {
			if ((int)(b->textT * TEXT_CPS) < (int)strlen(b->text)) { b->textT = 999; }
			else { GoTo(b, b->afterText); }
		}
		break;
	case B_ENEMY_DIALOGUE:
		if (b->stateT >= DIALOGUE_TIME) { GoTo(b, B_ENEMY_ATTACK); }
		break;
	case B_ENEMY_ATTACK:
		UpdateEnemyTurn(b, dt, plrHp);
		break;
	case B_LOST:
		for (int i = 0; i < MAX_SHARDS; i++) {
			b->shards[i].x += b->shardVel[i].x * dt;
			b->shards[i].y += b->shardVel[i].y * dt;
			b->shardVel[i].y += 600.0f * dt;
		}
		if (b->stateT > 2.0f && (Confirm() || IsKeyPressed(KEY_R))) { b->restartRequested = true; }
		break;
	default:
		break;
	}
}

static void DrawTyped(const char* s, float t, int x, int y, int fs, Color c) {
	int n = (int)(t * TEXT_CPS);
	int len = (int)strlen(s);
	if (n > len) { n = len; }
	DrawText(TextSubtext(s, 0, n), x, y, fs, c);
}

static void DrawMeter(float x, float y, float w, float h, float ratio, Color fill) {
	ratio = Clampf(ratio, 0, 1);
	DrawRectangle((int)x - 2, (int)y - 2, (int)w + 4, (int)h + 4, BLACK);
	DrawRectangle((int)x, (int)y, (int)w, (int)h, (Color){70, 70, 70, 255});
	DrawRectangle((int)x, (int)y, (int)(w * ratio), (int)h, fill);
}

static void DrawEnemy(const Battle* b) {
	float alpha = 1.0f;
	if (b->outcome == BATTLE_WON || b->outcome == BATTLE_SPARED) { alpha = 1.0f - Clampf(b->outcomeT / 1.0f, 0, 1); }
	if (alpha <= 0) { return; }

	Vector2 c = EnemyCenter();
	if (b->enemy.shake > 0) { c.x += sinf(b->time * 90.0f) * b->enemy.shake; }

	EnemyLook look = {
		.flash = b->enemy.hitFlash > 0,
		.angry = EnemyAngry(b),
		.calm = EnemySpareable(b),
		.alert = b->state == B_ENEMY_DIALOGUE,
	};
	Enemy_Draw(b->def, c, EnemySize(b), b->time * 6.0f, look, alpha);
}

static void DrawPlayer(const Battle* b) {
	float scale = 150.0f / b->plrSrc.height;
	Rectangle dest = {300, 150, b->plrSrc.width * scale, 150};
	struct Rectangle plr_src;

	for (int i = 0; i < 6; i++) {
		DrawCircle((int)(dest.x + dest.width / 2), (int)(dest.y + 75), 170 - i * 26, Fade((Color){190, 150, 255, 255}, 0.07f));
	}
	DrawEllipse((int)(dest.x + dest.width / 2), (int)(dest.y + 150), (int)(dest.width / 2), 9, Fade(BLACK, 0.5f));
	DrawTexturePro(b->plrTexture, plr_src, dest, (Vector2){0, 0}, 0, WHITE);
}

static void DrawBackground(const Battle* b) {
	int sw = GetScreenWidth(), sh = GetScreenHeight();
	float scroll = fmodf(b->time * 20.0f, 40.0f);
	Color line = Fade((Color){160, 60, 220, 255}, 0.18f);
	for (int x = -40; x < sw + 40; x += 40) { DrawLineEx((Vector2){x + scroll, 0}, (Vector2){x + scroll, (float)sh}, 2, line); }
	for (int y = -40; y < sh + 40; y += 40) { DrawLineEx((Vector2){0, y + scroll}, (Vector2){(float)sw, y + scroll}, 2, line); }
}

static void DrawTpGauge(const Battle* b) {
	float x = 40, y = 130, w = 36, h = 300;
	DrawText("TP", (int)x + 4, (int)y - 36, 28, WHITE);
	DrawRectangle((int)x - 3, (int)y - 3, (int)w + 6, (int)h + 6, BLACK);
	DrawRectangle((int)x, (int)y, (int)w, (int)h, (Color){90, 30, 20, 255});
	float fill = h * b->tp / 100.0f;
	DrawRectangle((int)x, (int)(y + h - fill), (int)w, (int)fill, (Color){255, 160, 40, 255});
	float costY = y + h - h * TP_PACIFY_COST / 100.0f;
	DrawLine((int)x - 6, (int)costY, (int)(x + w + 6), (int)costY, Fade(WHITE, 0.6f));
	const char* label = b->tp >= 100.0f ? "MAX" : TextFormat("%d%%", (int)b->tp);
	DrawText(label, (int)x - 2, (int)(y + h + 12), 22, b->tp >= 100.0f ? YELLOW : WHITE);
}

static void DrawButtons(const Battle* b) {
	bool inEnemyTurn = b->state == B_ENEMY_DIALOGUE || b->state == B_ENEMY_ATTACK || b->state == B_LOST || b->state == B_ATTACK_ANIM || b->state == B_TEXT;
	for (int i = 0; i < BUTTON_COUNT; i++) {
		Rectangle r = ButtonRect(i);
		bool selected = !inEnemyTurn && i == b->menuIndex;
		Color base = selected ? YELLOW : (Color){230, 120, 40, 255};
		if (inEnemyTurn) { base = (Color){110, 70, 40, 255}; }
		DrawRectangleRec(r, (Color){15, 5, 30, 255});
		DrawRectangleLinesEx(r, 3, base);
		int fs = 22;
		int tx = (int)(r.x + r.width / 2) - MeasureText(BUTTON_LABELS[i], fs) / 2 + (selected ? 12 : 0);
		DrawText(BUTTON_LABELS[i], tx, (int)(r.y + r.height / 2) - fs / 2, fs, base);
		if (selected) { Encounter_DrawSoul((Vector2){r.x + 26, r.y + r.height / 2}, RED); }
	}
}

static void DrawPanel(int plrHp) {
	Rectangle p = PanelRect();
	DrawRectangleRec(p, (Color){15, 5, 30, 255});
	DrawRectangle((int)p.x, (int)p.y, (int)p.width, 4, (Color){160, 60, 220, 255});
	DrawText("VOCE", (int)p.x + 24, (int)p.y + 32, 30, WHITE);
	DrawText("HP", (int)p.x + 160, (int)p.y + 38, 22, (Color){255, 120, 120, 255});
	float ratio = (float)plrHp / PLAYER_MAX_HP;
	DrawMeter(p.x + 195, p.y + 36, 160, 22, ratio, ratio > 0.3f ? (Color){80, 220, 90, 255} : ORANGE);
	DrawText(TextFormat("%d/%d", plrHp < 0 ? 0 : plrHp, PLAYER_MAX_HP), (int)p.x + 370, (int)p.y + 36, 24, WHITE);
}

static void DrawBox(const Battle* b) {
	Rectangle r = b->box;
	DrawRectangleRec(r, BLACK);
	Color border = b->hurtFlash > 0 ? RED : WHITE;
	DrawRectangleLinesEx(r, 5, border);
}

static void DrawEnemyRow(const Battle* b, Rectangle box) {
	int y = (int)box.y + 60;
	Color nameColor = EnemySpareable(b) ? YELLOW : WHITE;
	DrawText(TextFormat("* %s", b->def->name), (int)box.x + 90, y, 32, nameColor);
	Encounter_DrawSoul((Vector2){box.x + 55, (float)y + 17}, RED);

	float hpRatio = (float)b->enemy.hp / b->enemy.maxHp;
	DrawText("HP", (int)box.x + 420, y + 6, 22, WHITE);
	DrawMeter(box.x + 460, (float)y + 6, 150, 22, hpRatio, RED);
	DrawText(TextFormat("%d%%", (int)(hpRatio * 100)), (int)box.x + 625, y + 6, 22, WHITE);

	DrawText("MERCY", (int)box.x + 720, y + 6, 22, WHITE);
	DrawMeter(box.x + 810, (float)y + 6, 150, 22, b->enemy.mercy / 100.0f, YELLOW);
	DrawText(TextFormat("%d%%", (int)b->enemy.mercy), (int)box.x + 975, y + 6, 22, WHITE);
}

static Color ListItemColor(const Battle* b, int i) {
	if (b->state == B_ACT_LIST && i == PACIFY_INDEX) {
		return b->tp >= TP_PACIFY_COST ? (Color){255, 130, 200, 255} : GRAY;
	}
	if (b->state == B_SPARE_LIST) {
		if (i == 0 && EnemySpareable(b)) { return YELLOW; }
		if (i == 1 && b->def->isBoss) { return GRAY; }
	}
	return WHITE;
}

static const char* ListItemLabel(const Battle* b, int i) {
	if (b->state == B_ACT_LIST) { return ActLabel(b, i); }
	if (b->state == B_ITEM_LIST) {
		const BattleItem* it = &b->items[ItemSlot(b, i)];
		return TextFormat("%s (+%d HP) x%d", it->name, it->heal, it->count);
	}
	return SPARE_LABELS[i];
}

static void DrawList(const Battle* b, Rectangle box) {
	int count = ListCount(b);
	for (int i = 0; i < count; i++) {
		int y = (int)box.y + 22 + i * 38;
		int x = (int)box.x + 90;

		DrawText(TextFormat("* %s", ListItemLabel(b, i)), x, y, 30, ListItemColor(b, i));
		if (b->state == B_ACT_LIST && i == PACIFY_INDEX) {
			DrawText(TextFormat("%d%% TP", (int)TP_PACIFY_COST), x + 300, y + 6, 22, (Color){255, 160, 40, 255});
		}
		if (i == b->listIndex) { Encounter_DrawSoul((Vector2){(float)x - 35, (float)y + 16}, RED); }
	}
}

static void DrawAttackBar(const Battle* b, Rectangle box) {
	float bx = box.x + 60, bw = box.width - 120;
	float by = box.y + box.height / 2 - 30, bh = 60;

	DrawRectangle((int)bx, (int)by, (int)bw, (int)bh, (Color){20, 20, 30, 255});
	DrawRectangle((int)(bx + bw * 0.35f), (int)by, (int)(bw * 0.30f), (int)bh, (Color){90, 60, 20, 255});
	DrawRectangle((int)(bx + bw * 0.47f), (int)by, (int)(bw * 0.06f), (int)bh, (Color){200, 150, 40, 255});
	DrawRectangleLinesEx((Rectangle){bx, by, bw, bh}, 3, WHITE);
	DrawLine((int)(bx + bw / 2), (int)by - 6, (int)(bx + bw / 2), (int)(by + bh + 6), YELLOW);

	float mx = bx + bw * b->attackP;
	bool locked = b->state == B_ATTACK_ANIM;
	Color mc = locked && (int)(b->stateT * 20) % 2 == 0 ? YELLOW : WHITE;
	DrawRectangle((int)mx - 5, (int)by - 10, 10, (int)bh + 20, mc);

	if (b->state == B_ATTACK) { DrawText("[Z] ATACAR", (int)(box.x + box.width / 2) - 70, (int)(box.y + box.height - 34), 22, LIGHTGRAY); }
}

static void DrawAttackAnim(const Battle* b) {
	float left = EnemyLeft(b), top = EnemyTop(b), size = EnemySize(b);

	if (!b->attackMissed) {
		for (int i = 0; i < 3; i++) {
			float start = 0.1f + i * 0.08f;
			if (b->stateT < start || b->stateT > start + 0.25f) { continue; }
			float a = 1.0f - (b->stateT - start) / 0.25f;
			float ox = left + 15 + i * size * 0.35f;
			DrawLineEx((Vector2){ox, top - 10}, (Vector2){ox + 44, top + size + 10}, 8, Fade(WHITE, a));
		}
	}

	if (b->stateT >= 0.35f && b->stateT < 1.3f) {
		float hpRatio = (float)b->enemy.hp / b->enemy.maxHp;
		DrawMeter(left - 20, top - 56, size + 40, 18, hpRatio, RED);
	}

	if (b->dmgNumT > 0) {
		float rise = (1.0f - b->dmgNumT) * 30.0f;
		const char* txt = b->attackMissed ? "MISS" : TextFormat("%d", b->lastDamage);
		Color c = b->attackMissed ? LIGHTGRAY : (Color){255, 90, 90, 255};
		float cx = EnemyCenter().x;
		DrawText(txt, (int)cx - MeasureText(txt, 44) / 2, (int)(top - 112 - rise), 44, Fade(c, Clampf(b->dmgNumT * 2.0f, 0, 1)));
	}
}

static void DrawBubble(const Battle* b) {
	Vector2 c = EnemyCenter();
	Rectangle r = {c.x - EnemySize(b) * 0.85f - 320, EnemyTop(b) + 5, 290, 76};
	DrawRectangleRounded(r, 0.3f, 8, WHITE);
	DrawTriangle((Vector2){r.x + r.width - 2, r.y + 18}, (Vector2){r.x + r.width - 2, r.y + 50}, (Vector2){r.x + r.width + 26, r.y + 36}, WHITE);
	DrawTyped(b->line, b->stateT, (int)r.x + 20, (int)r.y + 24, 28, BLACK);
}

#define HEART_COLOR    (Color){240, 50, 90, 255}
#define SPADE_COLOR    (Color){170, 110, 255, 255}
#define CLUB_COLOR     (Color){80, 220, 120, 255}
#define DIAMOND_COLOR  (Color){80, 200, 255, 255}

static Vector2 LocalPt(Vector2 o, float ang, float x, float y) {
	float c = cosf(ang), s = sinf(ang);
	return (Vector2){o.x + x * c - y * s, o.y + x * s + y * c};
}

static void Tri(Vector2 a, Vector2 c, Vector2 d, Color col) {
	float cross = (c.x - a.x) * (d.y - a.y) - (c.y - a.y) * (d.x - a.x);
	if (cross > 0) { DrawTriangle(a, d, c, col); }
	else { DrawTriangle(a, c, d, col); }
}

static bool Blink(float t, float fuse) {
	float rate = 8.0f + 22.0f * Clampf(t / fuse, 0, 1);
	return sinf(t * rate) > -0.3f;
}

static void DrawHeartShape(Vector2 c, float s, Color col) {
	DrawCircleV((Vector2){c.x - s * 0.5f, c.y - s * 0.2f}, s * 0.55f, col);
	DrawCircleV((Vector2){c.x + s * 0.5f, c.y - s * 0.2f}, s * 0.55f, col);
	Tri((Vector2){c.x - s * 1.02f, c.y}, (Vector2){c.x + s * 1.02f, c.y}, (Vector2){c.x, c.y + s * 1.15f}, col);
}

static void DrawDiamondShape(Vector2 c, float s, Color col) {
	Vector2 top = {c.x, c.y - s * 1.3f}, bot = {c.x, c.y + s * 1.3f};
	Vector2 left = {c.x - s, c.y}, right = {c.x + s, c.y};
	Tri(top, left, right, col);
	Tri(bot, left, right, col);
}

static void DrawSpadeHalf(Vector2 c, float s, float ang, float side, float open, Color col) {
	float o = open * side;
	DrawCircleV(LocalPt(c, ang, side * 0.5f * s + o, 0.2f * s), 0.55f * s, col);
	Tri(LocalPt(c, ang, side * 1.02f * s + o, 0), LocalPt(c, ang, o, 0), LocalPt(c, ang, o, -1.15f * s), col);
	Tri(LocalPt(c, ang, o, 0.25f * s), LocalPt(c, ang, side * 0.35f * s + o, 0.9f * s), LocalPt(c, ang, o, 0.9f * s), col);
}

static void DrawClubShape(Vector2 c, float s, float ang, Color col) {
	DrawCircleV(LocalPt(c, ang, 0, -0.45f * s), 0.42f * s, col);
	DrawCircleV(LocalPt(c, ang, -0.45f * s, 0.1f * s), 0.42f * s, col);
	DrawCircleV(LocalPt(c, ang, 0.45f * s, 0.1f * s), 0.42f * s, col);
	DrawCircleV(c, 0.3f * s, col);
	Tri(LocalPt(c, ang, 0, 0.1f * s), LocalPt(c, ang, -0.3f * s, 0.85f * s), LocalPt(c, ang, 0.3f * s, 0.85f * s), col);
}

static void DrawHeartHazard(const Hazard* h) {
	float s = 13.0f + 2.5f * sinf(h->t * 14.0f) + 4.0f * (h->t / HEART_FUSE);
	float a = Blink(h->t, HEART_FUSE) ? 1.0f : 0.3f;
	DrawHeartShape(h->pos, s + 3, Fade(WHITE, a * 0.6f));
	DrawHeartShape(h->pos, s, Fade(HEART_COLOR, a));
}

static void DrawSpadeLaser(const Battle* b, const Hazard* h) {
	bool hurts;
	float w = SpadeLaserWidth(h, &hurts);
	Vector2 end = SpadeLaserEnd(h);

	if (h->t < SPADE_APPEAR) {
		DrawLineEx(h->pos, end, 2, Fade(SPADE_COLOR, Blink(h->t, SPADE_APPEAR) ? 0.45f : 0.15f));
	}
	if (w <= 0) { return; }
	if (hurts) { w += sinf(b->time * 60.0f) * 2.0f; }
	DrawLineEx(h->pos, end, w, SPADE_COLOR);
	DrawLineEx(h->pos, end, w * 0.45f, WHITE);
}

static void DrawSpadeBody(const Hazard* h) {
	float s = 16.0f * Clampf(h->t / 0.2f, 0, 1);
	float t0 = SPADE_APPEAR, t1 = t0 + SPADE_OPEN, t2 = t1 + SPADE_FIRE;
	float maxOpen = s * 0.45f;
	float open = 0, alpha = 1;

	if (h->t < t0) { alpha = Blink(h->t, t0) ? 1.0f : 0.45f; }
	else if (h->t < t1) { open = maxOpen * (h->t - t0) / SPADE_OPEN; }
	else if (h->t < t2) { open = maxOpen; }
	else {
		float k = Clampf((h->t - t2) / SPADE_CLOSE, 0, 1);
		open = maxOpen * (1.0f - k);
		alpha = 1.0f - k;
	}

	float ang = h->dir + PI / 2;
	DrawSpadeHalf(h->pos, s, ang, -1, open, Fade(SPADE_COLOR, alpha));
	DrawSpadeHalf(h->pos, s, ang, 1, open, Fade(SPADE_COLOR, alpha));
}

static void DrawClubHazard(const Hazard* h) {
	float s = 14.0f * Clampf(h->t / 0.15f, 0, 1);
	float bodyA = h->t < CLUB_APPEAR ? (Blink(h->t, CLUB_APPEAR) ? 1.0f : 0.45f) : 1.0f;

	ClubRods r = ClubGeometry(h);
	if (h->t >= CLUB_HIT_T) { bodyA = r.alpha; }

	for (int k = 0; k < r.count; k++) {
		Color c = Fade((Color){210, 255, 220, 255}, r.alpha);
		DrawLineEx(r.from[k], r.to[k], r.width, c);
		DrawCircleV(r.to[k], r.width * 0.5f, c);
	}
	DrawClubShape(h->pos, s, h->dir + PI / 2, Fade(CLUB_COLOR, bodyA));
}

static void DrawDiamondHazard(const Hazard* h) {
	float s = 14.0f + 2.0f * sinf(h->t * 16.0f);
	float a = Blink(h->t, DIAMOND_FUSE) ? 1.0f : 0.3f;
	DrawDiamondShape(h->pos, s + 3, Fade(WHITE, a * 0.6f));
	DrawDiamondShape(h->pos, s, Fade(DIAMOND_COLOR, a));
}

static void DrawPawWarning(const Battle* b, const Hazard* h) {
	bool on = (int)(b->time * 8.0f) % 2 == 0;
	DrawRectangleRec(h->area, Fade(RED, on ? 0.28f : 0.14f));
	DrawRectangleLinesEx(h->area, 3, Fade(RED, on ? 1.0f : 0.5f));
	const char* txt = "!!!!";
	int fs = 48;
	int tw = MeasureText(txt, fs);
	DrawText(txt, (int)(h->area.x + h->area.width / 2) - tw / 2, (int)(h->area.y + h->area.height / 2) - fs / 2, fs, on ? YELLOW : RED);
}

static void DrawPaw(const Hazard* h, float k) {
	Vector2 fwd = PawForward(h->side);
	Vector2 perp = {-fwd.y, fwd.x};
	bool horiz = h->side <= 1;
	float depth = horiz ? h->area.width : h->area.height;
	float edgeLen = horiz ? h->area.height : h->area.width;

	Vector2 back = {h->area.x + h->area.width / 2, h->area.y + h->area.height / 2};
	back.x -= fwd.x * depth / 2;
	back.y -= fwd.y * depth / 2;
	float slide = (1.0f - k) * depth * 1.2f;
	back.x -= fwd.x * slide;
	back.y -= fwd.y * slide;

	float toeR = fminf(edgeLen * 0.11f, depth * 0.22f);
	float toeLine = depth - toeR * 1.7f;
	int toes = edgeLen > 250 ? 6 : 4;

	Color scale = {120, 25, 45, 255};
	Color dark = {80, 15, 30, 255};
	Color claw = {245, 235, 210, 255};

	Vector2 p0 = {back.x - perp.x * edgeLen * 0.48f, back.y - perp.y * edgeLen * 0.48f};
	Vector2 p1 = {back.x + perp.x * edgeLen * 0.48f + fwd.x * toeLine, back.y + perp.y * edgeLen * 0.48f + fwd.y * toeLine};
	Rectangle palm = {fminf(p0.x, p1.x), fminf(p0.y, p1.y), fabsf(p1.x - p0.x), fabsf(p1.y - p0.y)};
	DrawRectangleRounded(palm, 0.35f, 8, scale);

	for (int i = 0; i < 5; i++) {
		for (int j = 1; j < 4; j++) {
			float u = (i + 0.5f) / 5.0f - 0.5f;
			float v = toeLine * j / 4.0f;
			Vector2 sc = {back.x + perp.x * edgeLen * 0.85f * u + fwd.x * v, back.y + perp.y * edgeLen * 0.85f * u + fwd.y * v};
			DrawCircleV(sc, toeR * 0.35f, dark);
		}
	}

	for (int i = 0; i < toes; i++) {
		float u = 0.12f + 0.76f * i / (toes - 1) - 0.5f;
		Vector2 tc = {back.x + perp.x * edgeLen * u + fwd.x * toeLine, back.y + perp.y * edgeLen * u + fwd.y * toeLine};
		DrawCircleV(tc, toeR, scale);
		DrawCircleV(tc, toeR * 0.55f, dark);

		Vector2 tip = {tc.x + fwd.x * toeR * 1.65f, tc.y + fwd.y * toeR * 1.65f};
		Vector2 bl = {tc.x + fwd.x * toeR * 0.5f + perp.x * toeR * 0.38f, tc.y + fwd.y * toeR * 0.5f + perp.y * toeR * 0.38f};
		Vector2 br = {tc.x + fwd.x * toeR * 0.5f - perp.x * toeR * 0.38f, tc.y + fwd.y * toeR * 0.5f - perp.y * toeR * 0.38f};
		Tri(tip, bl, br, claw);
	}
}

static void DrawHazards(const Battle* b) {
	Rectangle box = BulletBox();

	BeginScissorMode((int)box.x + 5, (int)box.y + 5, (int)box.width - 10, (int)box.height - 10);
	for (int i = 0; i < MAX_HAZARDS; i++) {
		const Hazard* h = &b->hazards[i];
		switch (h->kind) {
		case HZ_PAW: {
			bool hurts;
			float k = PawSlide(h, &hurts);
			if (h->t < PAW_WARN) { DrawPawWarning(b, h); }
			else if (k > 0) { DrawPaw(h, k); }
			break;
		}
		case HZ_SPADE:   DrawSpadeLaser(b, h); break;
		case HZ_CLUB:    DrawClubHazard(h); break;
		case HZ_HEART:   DrawHeartHazard(h); break;
		case HZ_DIAMOND: DrawDiamondHazard(h); break;
		default: break;
		}
	}
	EndScissorMode();

	for (int i = 0; i < MAX_HAZARDS; i++) {
		if (b->hazards[i].kind == HZ_SPADE) { DrawSpadeBody(&b->hazards[i]); }
	}
}

static void DrawBullets(const Battle* b) {
	for (int i = 0; i < MAX_BULLETS; i++) {
		const Bullet* bl = &b->bullets[i];
		if (!bl->active) { continue; }
		switch (bl->shape) {
		case BULLET_DIAMOND:
			DrawDiamondShape(bl->pos, bl->r, DIAMOND_COLOR);
			DrawDiamondShape(bl->pos, bl->r * 0.45f, WHITE);
			break;
		case BULLET_HEART:
			DrawCircleV(bl->pos, bl->r, HEART_COLOR);
			DrawCircleV(bl->pos, bl->r * 0.5f, WHITE);
			break;
		default: {
			Color outer = bl->r >= 11 ? (Color){255, 150, 40, 255} : (Color){255, 70, 90, 255};
			DrawCircleV(bl->pos, bl->r, outer);
			DrawCircleV(bl->pos, bl->r * 0.5f, WHITE);
			break;
		}
		}
	}
}

static void DrawBoxContents(const Battle* b) {
	Rectangle box = b->box;
	switch (b->state) {
	case B_MENU:
		DrawTyped(b->flavor, b->stateT, (int)box.x + 40, (int)box.y + 30, 32, WHITE);
		break;
	case B_FIGHT_SELECT:
	case B_ACT_SELECT:
		DrawEnemyRow(b, box);
		break;
	case B_ACT_LIST:
	case B_ITEM_LIST:
	case B_SPARE_LIST:
		DrawList(b, box);
		break;
	case B_ATTACK:
		DrawAttackBar(b, box);
		break;
	case B_ATTACK_ANIM:
		DrawAttackBar(b, box);
		DrawAttackAnim(b);
		break;
	case B_TEXT:
		DrawTyped(b->text, b->textT, (int)box.x + 40, (int)box.y + 30, 32, WHITE);
		break;
	case B_ENEMY_DIALOGUE:
		DrawBubble(b);
		break;
	case B_ENEMY_ATTACK: {
		DrawHazards(b);
		DrawBullets(b);
		bool blink = b->invuln > 0 && (int)(b->time * 20.0f) % 2 == 0;
		if (!blink) { Encounter_DrawSoul(b->soul, RED); }
		break;
	}
	default:
		break;
	}
}

static void DrawGameOver(const Battle* b) {
	int sw = GetScreenWidth(), sh = GetScreenHeight();

	if (b->stateT < 0.6f) {
		Encounter_DrawSoul(b->soul, RED);
	} else {
		for (int i = 0; i < MAX_SHARDS; i++) { DrawRectangle((int)b->shards[i].x - 4, (int)b->shards[i].y - 4, 8, 8, RED); }
	}

	float a = Clampf((b->stateT - 0.6f) / 1.0f, 0, 1);
	DrawRectangle(0, 0, sw, sh, Fade(BLACK, a * 0.96f));
	if (b->stateT <= 1.2f) { return; }

	const char* msg = "GAME OVER";
	DrawText(msg, sw / 2 - MeasureText(msg, 72) / 2, sh / 2 - 60, 72, RED);
	if (b->stateT > 2.0f) {
		const char* hint = "Aperte [ENTER] ou [R] para tentar de novo";
		DrawText(hint, sw / 2 - MeasureText(hint, 26) / 2, sh / 2 + 30, 26, LIGHTGRAY);
	}
}

void Battle_Draw(const Battle* b, int plrHp) {
	if (!b->active) { return; }

	ClearBackground(BLACK);
	DrawBackground(b);
	DrawTpGauge(b);
	DrawEnemy(b);
	DrawPlayer(b);
	DrawBox(b);
	DrawBoxContents(b);
	DrawButtons(b);
	DrawPanel(plrHp);

	if (b->state == B_LOST) { DrawGameOver(b); }
	if (b->state == B_FADE_IN) {
		DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 1.0f - Clampf(b->stateT / 0.5f, 0, 1)));
	}
}
