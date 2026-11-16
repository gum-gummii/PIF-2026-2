#include <battle.h>
#include <encounter.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define ENEMY_NAME        "DEMONINHO"
#define ENEMY_GRID        12
#define ENEMY_PIXEL       9
#define ENEMY_SIZE        (ENEMY_GRID * ENEMY_PIXEL)
#define ENEMY_MAX_HP      200
#define TEXT_CPS          42.0f
#define TP_PACIFY_COST    32.0f
#define SOUL_SPEED        230.0f
#define SOUL_HIT_R        5.0f
#define GRAZE_R           16.0f
#define INVULN_TIME       1.0f
#define ATTACK_TIME       1.15f
#define ENEMY_TURN_TIME   6.3f
#define SPAWN_STOP        5.4f
#define DIALOGUE_TIME     2.2f
#define MENU_LOCK_START   0.3f

static const char* BUTTON_LABELS[BUTTON_COUNT] = {"LUTAR", "AGIR", "ITEM", "POUPAR", "DEFENDER"};
static const char* ACT_LABELS[ACT_COUNT] = {"Checar", "Elogiar", "Danca", "Pacificar"};
static const char* SPARE_LABELS[2] = {"Poupar", "Fugir"};

static const char* ENEMY_SPRITE[ENEMY_GRID] = {
	"..H......H..",
	"..HH....HH..",
	"..PPPPPPPP..",
	".PPPPPPPPPP.",
	".PWEPPPPEWP.",
	".PPPPPPPPPP.",
	".PPMMMMMMPP.",
	"..PPPPPPPP..",
	".DDPPPPPPDD.",
	".D.PPPPPP.D.",
	"...PP..PP...",
	"..FFF..FFF..",
};

static const char* FLAVOR_TEXTS[3] = {
	"* Cheiro de enxofre no ar.",
	"* O DEMONINHO range os dentes.",
	"* Hora de decidir o que fazer.",
};
static const char* LINES_NORMAL[3] = {"Grrr!", "Me enfrente!", "Voce nao passa!"};
static const char* LINES_CALM[3] = {"...ta bom.", "Ate que voce e legal.", "Hmpf."};
static const char* LINES_ANGRY[3] = {"AGORA CHEGA!", "VOU TE QUEIMAR!", "RAAAH!"};

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

static Vector2 EnemyPos(void) {
	return (Vector2){GetScreenWidth() * 0.66f, 150};
}

static Vector2 EnemyCenter(void) {
	Vector2 p = EnemyPos();
	return (Vector2){p.x + ENEMY_SIZE / 2.0f, p.y + ENEMY_SIZE / 2.0f};
}

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

static void AddBullet(Battle* b, Vector2 pos, Vector2 vel, float r, int dmg) {
	for (int i = 0; i < MAX_BULLETS; i++) {
		if (b->bullets[i].active) { continue; }
		b->bullets[i] = (Bullet){true, pos, vel, r, dmg, false};
		return;
	}
}

static void PickFlavor(Battle* b) {
	const char* msg;
	if (b->turn == 0) { msg = "* O DEMONINHO te encara com raiva."; }
	else if (EnemySpareable(b)) { msg = "* O DEMONINHO parece pronto para ser poupado."; }
	else if (EnemyAngry(b)) { msg = "* O DEMONINHO esta furioso!"; }
	else { msg = FLAVOR_TEXTS[b->turn % 3]; }
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
		const char** set = EnemyAngry(b) ? LINES_ANGRY : (b->enemy.mercy >= 50.0f ? LINES_CALM : LINES_NORMAL);
		snprintf(b->line, sizeof(b->line), "%s", set[GetRandomValue(0, 2)]);
		break;
	}
	case B_ENEMY_ATTACK: {
		Rectangle box = BulletBox();
		b->soul = (Vector2){box.x + box.width / 2, box.y + box.height / 2};
		memset(b->bullets, 0, sizeof(b->bullets));
		b->pattern = GetRandomValue(0, 2);
		if (b->pattern == b->lastPattern) { b->pattern = (b->pattern + 1) % 3; }
		b->lastPattern = b->pattern;
		b->spawnT = 0.3f;
		b->invuln = 0;
		break;
	}
	case B_LOST:
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

static void Finish(Battle* b, int outcome, const char* msg) {
	b->outcome = outcome;
	b->outcomeT = 0;
	SetText(b, msg, B_DONE);
}

void Battle_Start(Battle* b, Texture2D plrTexture, Rectangle plrSrc) {
	memset(b, 0, sizeof(Battle));
	b->active = true;
	b->plrTexture = plrTexture;
	b->plrSrc = plrSrc;
	b->enemy.hp = b->enemy.maxHp = ENEMY_MAX_HP;
	b->items[0] = (BattleItem){"Bolo", 40, 2};
	b->items[1] = (BattleItem){"Suco", 20, 3};
	b->items[2] = (BattleItem){"Biscoito", 10, 4};
	b->lastPattern = -1;
	b->box = IdleBox();
	b->menuLock = MENU_LOCK_START;
	b->state = B_FADE_IN;
	b->stateT = 0;
}

void Battle_Stop(Battle* b) { b->active = false; }
bool Battle_IsActive(const Battle* b) { return b->active; }

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

static void SpawnPattern(Battle* b) {
	Rectangle box = BulletBox();
	float spd = EnemyAngry(b) ? 1.25f : 1.0f;

	if (b->pattern == 0) {
		float x = (float)GetRandomValue((int)box.x + 12, (int)(box.x + box.width) - 12);
		AddBullet(b, (Vector2){x, box.y - 20}, (Vector2){0, 210 * spd}, 9, 9);
		b->spawnT = 0.2f / spd;
	} else if (b->pattern == 1) {
		bool right = GetRandomValue(0, 1) == 1;
		float y = (float)GetRandomValue((int)box.y + 20, (int)(box.y + box.height) - 20);
		float x = right ? box.x + box.width + 30 : box.x - 30;
		AddBullet(b, (Vector2){x, y}, (Vector2){(right ? -270.0f : 270.0f) * spd, 0}, 10, 9);
		b->spawnT = 0.6f / spd;
	} else {
		Vector2 ec = EnemyCenter();
		Vector2 aim = Norm((Vector2){b->soul.x - ec.x, b->soul.y - ec.y});
		float base = atan2f(aim.y, aim.x);
		for (int i = -2; i <= 2; i++) {
			float a = base + i * 0.22f;
			AddBullet(b, ec, (Vector2){cosf(a) * 240 * spd, sinf(a) * 240 * spd}, 11, 12);
		}
		b->spawnT = 1.0f / spd;
	}
}

static void UpdateEnemyTurn(Battle* b, float dt, int* plrHp) {
	SoulMove(b, dt);

	if (b->stateT < SPAWN_STOP) {
		b->spawnT -= dt;
		if (b->spawnT <= 0) { SpawnPattern(b); }
	}

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
			int dmg = b->defending ? (bl->dmg + 1) / 2 : bl->dmg;
			*plrHp -= dmg;
			if (*plrHp < 0) { *plrHp = 0; }
			b->invuln = INVULN_TIME;
			b->hurtFlash = 0.25f;
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

	if (b->stateT >= ENEMY_TURN_TIME) {
		memset(b->bullets, 0, sizeof(b->bullets));
		b->defending = false;
		b->turn++;
		GoTo(b, B_MENU);
	}
}

static void DoAct(Battle* b, int act) {
	b->tp = Clampf(b->tp + 4, 0, 100);

	if (act == 0) {
		SetText(b, "* DEMONINHO - ATQ 10 DEF 5\n* Parece bravo, mas gosta de atencao.", B_ENEMY_DIALOGUE);
		return;
	}

	if (act == 1) {
		b->enemy.mercy += 35;
		SetText(b, "* Voce elogiou o DEMONINHO.\n* Ele ficou meio sem jeito.", B_ENEMY_DIALOGUE);
	} else if (act == 2) {
		b->enemy.mercy += 25;
		SetText(b, "* Voce dancou.\n* O DEMONINHO tentou acompanhar.", B_ENEMY_DIALOGUE);
	} else {
		b->tp -= TP_PACIFY_COST;
		b->enemy.mercy += 60;
		SetText(b, "* Voce lancou PACIFICAR!\n* O DEMONINHO ficou mais calmo.", B_ENEMY_DIALOGUE);
	}
	b->enemy.mercy = Clampf(b->enemy.mercy, 0, 100);
}

static void UpdateMenu(Battle* b, bool ok, bool back, int* plrHp) {
	if (b->state == B_MENU) {
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
		return;
	}

	if (back) {
		GoTo(b, B_MENU);
		return;
	}

	int count = 1;
	if (b->state == B_ACT_LIST) { count = ACT_COUNT; }
	else if (b->state == B_ITEM_LIST) { count = ItemTypes(b); }
	else if (b->state == B_SPARE_LIST) { count = 2; }

	if (NavUp()) { b->listIndex = (b->listIndex + count - 1) % count; }
	if (NavDown()) { b->listIndex = (b->listIndex + 1) % count; }
	if (!ok) { return; }

	if (b->state == B_FIGHT_SELECT) {
		GoTo(b, B_ATTACK);
	} else if (b->state == B_ACT_SELECT) {
		GoTo(b, B_ACT_LIST);
	} else if (b->state == B_ACT_LIST) {
		if (b->listIndex == 3 && b->tp < TP_PACIFY_COST) { return; }
		DoAct(b, b->listIndex);
	} else if (b->state == B_ITEM_LIST) {
		int slot = ItemSlot(b, b->listIndex);
		if (slot < 0) { return; }
		BattleItem* it = &b->items[slot];
		int before = *plrHp;
		*plrHp = (int)Clampf((float)(*plrHp + it->heal), 0, PLAYER_MAX_HP);
		it->count--;
		char msg[128];
		snprintf(msg, sizeof(msg), "* Voce usou %s.\n* Recuperou %d HP!", it->name, *plrHp - before);
		SetText(b, msg, B_ENEMY_DIALOGUE);
	} else if (b->state == B_SPARE_LIST) {
		if (b->listIndex == 1) {
			Finish(b, 3, "* Voce fugiu...");
		} else if (EnemySpareable(b)) {
			Finish(b, 2, "* Voce poupou o DEMONINHO.\n* Ganhou 0 EXP e 15 D$.");
		} else {
			SetText(b, "* Voce poupou o DEMONINHO...\n* Mas ele ainda nao quer parar.", B_ENEMY_DIALOGUE);
		}
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

	if (b->stateT >= 1.3f) {
		if (b->enemy.hp <= 0) {
			Finish(b, 1, "* VOCE VENCEU!\n* Ganhou 30 EXP e 20 D$.");
		} else {
			GoTo(b, B_ENEMY_DIALOGUE);
		}
	}
}

void Battle_Update(Battle* b, float dt, int* plrHp) {
	if (!b->active) { return; }

	b->stateT += dt;
	b->time += dt;
	if (b->menuLock > 0) { b->menuLock -= dt; }
	if (b->enemy.hitFlash > 0) { b->enemy.hitFlash -= dt; }
	if (b->enemy.shake > 0) { b->enemy.shake = fmaxf(0, b->enemy.shake - 40.0f * dt); }
	if (b->dmgNumT > 0) { b->dmgNumT -= dt; }
	if (b->invuln > 0) { b->invuln -= dt; }
	if (b->hurtFlash > 0) { b->hurtFlash -= dt; }
	if (b->outcome != 0) { b->outcomeT += dt; }

	bool toBullet = b->state == B_ENEMY_DIALOGUE || b->state == B_ENEMY_ATTACK;
	Rectangle target = toBullet ? BulletBox() : IdleBox();
	float k = 1.0f - expf(-14.0f * dt);
	b->box.x += (target.x - b->box.x) * k;
	b->box.y += (target.y - b->box.y) * k;
	b->box.width += (target.width - b->box.width) * k;
	b->box.height += (target.height - b->box.height) * k;

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

static Color EnemyColor(char ch, const Battle* b) {
	bool angry = EnemyAngry(b);
	bool calm = EnemySpareable(b);
	switch (ch) {
		case 'H': return (Color){240, 220, 140, 255};
		case 'P': return calm ? (Color){230, 200, 70, 255} : (angry ? (Color){170, 60, 110, 255} : (Color){130, 70, 190, 255});
		case 'D': return calm ? (Color){170, 140, 40, 255} : (angry ? (Color){110, 30, 70, 255} : (Color){80, 40, 130, 255});
		case 'W': return WHITE;
		case 'E': return (b->state == B_ENEMY_DIALOGUE && angry) ? RED : BLACK;
		case 'M': return (Color){90, 10, 30, 255};
		case 'F': return (Color){40, 20, 60, 255};
		default:  return BLANK;
	}
}

static void DrawEnemy(const Battle* b) {
	float alpha = 1.0f;
	if (b->outcome == 1 || b->outcome == 2) { alpha = 1.0f - Clampf(b->outcomeT / 1.0f, 0, 1); }
	if (alpha <= 0) { return; }

	Vector2 pos = EnemyPos();
	float shake = b->enemy.shake > 0 ? sinf(b->time * 90.0f) * b->enemy.shake : 0;
	bool flash = b->enemy.hitFlash > 0;

	DrawEllipse((int)(pos.x + ENEMY_SIZE / 2.0f), (int)(pos.y + ENEMY_SIZE), ENEMY_SIZE / 2, 10, Fade(BLACK, 0.35f * alpha));

	for (int row = 0; row < ENEMY_GRID; row++) {
		for (int col = 0; col < ENEMY_GRID; col++) {
			char ch = ENEMY_SPRITE[row][ENEMY_GRID - 1 - col];
			if (ch == '.') { continue; }
			Color c = flash ? WHITE : EnemyColor(ch, b);
			DrawRectangle((int)(pos.x + shake) + col * ENEMY_PIXEL, (int)pos.y + row * ENEMY_PIXEL, ENEMY_PIXEL, ENEMY_PIXEL, Fade(c, alpha));
		}
	}
}

static void DrawPlayer(const Battle* b) {
	float scale = 150.0f / b->plrSrc.height;
	Rectangle dest = {300, 150, b->plrSrc.width * scale, 150};
	for (int i = 0; i < 6; i++) {
		DrawCircle((int)(dest.x + dest.width / 2), (int)(dest.y + 75), 170 - i * 26, Fade((Color){190, 150, 255, 255}, 0.07f));
	}
	DrawEllipse((int)(dest.x + dest.width / 2), (int)(dest.y + 150), (int)(dest.width / 2), 9, Fade(BLACK, 0.5f));
	DrawTexturePro(b->plrTexture, b->plrSrc, dest, (Vector2){0, 0}, 0, WHITE);
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
	DrawText("* " ENEMY_NAME, (int)box.x + 90, y, 32, nameColor);
	Encounter_DrawSoul((Vector2){box.x + 55, (float)y + 17}, RED);

	float hpRatio = (float)b->enemy.hp / b->enemy.maxHp;
	DrawText("HP", (int)box.x + 420, y + 6, 22, WHITE);
	DrawMeter(box.x + 460, (float)y + 6, 150, 22, hpRatio, RED);
	DrawText(TextFormat("%d%%", (int)(hpRatio * 100)), (int)box.x + 625, y + 6, 22, WHITE);

	DrawText("MERCY", (int)box.x + 720, y + 6, 22, WHITE);
	DrawMeter(box.x + 810, (float)y + 6, 150, 22, b->enemy.mercy / 100.0f, YELLOW);
	DrawText(TextFormat("%d%%", (int)b->enemy.mercy), (int)box.x + 975, y + 6, 22, WHITE);
}

static void DrawList(const Battle* b, Rectangle box) {
	int count = 1;
	if (b->state == B_ACT_LIST) { count = ACT_COUNT; }
	else if (b->state == B_ITEM_LIST) { count = ItemTypes(b); }
	else if (b->state == B_SPARE_LIST) { count = 2; }

	for (int i = 0; i < count; i++) {
		int y = (int)box.y + 22 + i * 38;
		int x = (int)box.x + 90;
		Color c = WHITE;
		const char* label = "";

		if (b->state == B_ACT_LIST) {
			label = ACT_LABELS[i];
			if (i == 3) { c = b->tp >= TP_PACIFY_COST ? (Color){255, 130, 200, 255} : GRAY; }
		} else if (b->state == B_ITEM_LIST) {
			const BattleItem* it = &b->items[ItemSlot(b, i)];
			label = TextFormat("%s (+%d HP) x%d", it->name, it->heal, it->count);
		} else {
			label = SPARE_LABELS[i];
			if (i == 0 && EnemySpareable(b)) { c = YELLOW; }
		}

		DrawText(TextFormat("* %s", label), x, y, 30, c);
		if (b->state == B_ACT_LIST && i == 3) {
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
	Vector2 pos = EnemyPos();

	if (!b->attackMissed) {
		for (int i = 0; i < 3; i++) {
			float start = 0.1f + i * 0.08f;
			if (b->stateT < start || b->stateT > start + 0.25f) { continue; }
			float a = 1.0f - (b->stateT - start) / 0.25f;
			float ox = pos.x + 15 + i * 38;
			DrawLineEx((Vector2){ox, pos.y - 10}, (Vector2){ox + 44, pos.y + ENEMY_SIZE + 10}, 8, Fade(WHITE, a));
		}
	}

	if (b->stateT >= 0.35f && b->stateT < 1.3f) {
		float hpRatio = (float)b->enemy.hp / b->enemy.maxHp;
		DrawMeter(pos.x - 20, pos.y - 56, ENEMY_SIZE + 40, 18, hpRatio, RED);
	}

	if (b->dmgNumT > 0) {
		float rise = (1.0f - b->dmgNumT) * 30.0f;
		const char* txt = b->attackMissed ? "MISS" : TextFormat("%d", b->lastDamage);
		Color c = b->attackMissed ? LIGHTGRAY : (Color){255, 90, 90, 255};
		DrawText(txt, (int)(pos.x + ENEMY_SIZE / 2.0f) - MeasureText(txt, 44) / 2, (int)(pos.y - 112 - rise), 44, Fade(c, Clampf(b->dmgNumT * 2.0f, 0, 1)));
	}
}

static void DrawBubble(const Battle* b) {
	Vector2 pos = EnemyPos();
	Rectangle r = {pos.x - 330, pos.y + 5, 290, 76};
	DrawRectangleRounded(r, 0.3f, 8, WHITE);
	DrawTriangle((Vector2){r.x + r.width - 2, r.y + 18}, (Vector2){r.x + r.width - 2, r.y + 50}, (Vector2){r.x + r.width + 26, r.y + 36}, WHITE);
	DrawTyped(b->line, b->stateT, (int)r.x + 20, (int)r.y + 24, 28, BLACK);
}

static void DrawBullets(const Battle* b) {
	for (int i = 0; i < MAX_BULLETS; i++) {
		const Bullet* bl = &b->bullets[i];
		if (!bl->active) { continue; }
		Color outer = bl->dmg >= 12 ? (Color){255, 150, 40, 255} : (Color){255, 70, 90, 255};
		DrawCircleV(bl->pos, bl->r, outer);
		DrawCircleV(bl->pos, bl->r * 0.5f, WHITE);
	}
}

void Battle_Draw(const Battle* b, int plrHp) {
	if (!b->active) { return; }
	int sw = GetScreenWidth(), sh = GetScreenHeight();

	ClearBackground(BLACK);
	DrawBackground(b);
	DrawTpGauge(b);
	DrawEnemy(b);
	DrawPlayer(b);
	DrawBox(b);

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
		DrawBullets(b);
		bool blink = b->invuln > 0 && (int)(b->time * 20.0f) % 2 == 0;
		if (!blink) { Encounter_DrawSoul(b->soul, RED); }
		break;
	}
	default:
		break;
	}

	DrawButtons(b);
	DrawPanel(plrHp);

	if (b->state == B_LOST) {
		if (b->stateT < 0.6f) {
			Encounter_DrawSoul(b->soul, RED);
		} else {
			for (int i = 0; i < MAX_SHARDS; i++) { DrawRectangle((int)b->shards[i].x - 4, (int)b->shards[i].y - 4, 8, 8, RED); }
		}
		float a = Clampf((b->stateT - 0.6f) / 1.0f, 0, 1);
		DrawRectangle(0, 0, sw, sh, Fade(BLACK, a * 0.96f));
		if (b->stateT > 1.2f) {
			const char* msg = "GAME OVER";
			DrawText(msg, sw / 2 - MeasureText(msg, 72) / 2, sh / 2 - 60, 72, RED);
			if (b->stateT > 2.0f) {
				const char* hint = "Aperte [ENTER] ou [R] para tentar de novo";
				DrawText(hint, sw / 2 - MeasureText(hint, 26) / 2, sh / 2 + 30, 26, LIGHTGRAY);
			}
		}
	}

	if (b->state == B_FADE_IN) { DrawRectangle(0, 0, sw, sh, Fade(BLACK, 1.0f - Clampf(b->stateT / 0.5f, 0, 1))); }
}
