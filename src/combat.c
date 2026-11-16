#include <combat.h>
#include <math.h>
#include <string.h>

// constantes
#define ENEMY_PIXEL        4          // cada "pixel" do sprite = 4x4 px na tela
#define ENEMY_GRID         12
#define ENEMY_SIZE         (ENEMY_GRID * ENEMY_PIXEL)
#define ENEMY_MAX_HP       100
#define ENEMY_SPEED        260.0f     // px/s

#define PLAYER_MELEE_DMG   12
#define PLAYER_MELEE_CD    0.35f
#define PLAYER_MELEE_REACH 95.0f
#define PLAYER_MELEE_WIDTH 80.0f
#define PLAYER_SHOT_DMG    6
#define PLAYER_SHOT_CD     0.45f
#define PLAYER_SHOT_SPEED  900.0f
#define PLAYER_INVULN      0.7f

#define ENEMY_MELEE_DMG    14
#define ENEMY_SHOT_DMG     8
#define ENEMY_SHOT_SPEED   420.0f

// Sprite do inimigo desenhado so com retangulos.
// H=chifre  P=corpo  D=corpo escuro  W=branco do olho  E=pupila  M=boca  F=pe
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

//utilidades
static Vector2 Norm(Vector2 v) {
	float len = sqrtf(v.x * v.x + v.y * v.y);
	if (len < 0.0001f) { return (Vector2){0, 0}; }
	return (Vector2){v.x / len, v.y / len};
}

static Vector2 RectCenter(Rectangle r) { return (Vector2){r.x + r.width / 2, r.y + r.height / 2}; }

static Rectangle EnemyBody(const Enemy* e) { return (Rectangle){e->pos.x, e->pos.y, ENEMY_SIZE, ENEMY_SIZE}; }

// hitbox do player um pouco menor que o frame inteiro 
static Rectangle PlayerHurtbox(Rectangle body) {
	float mx = body.width * 0.2f, my = body.height * 0.15f;
	return (Rectangle){body.x + mx, body.y + my, body.width - mx * 2, body.height - my * 2};
}

// retangulo de ataque corpo a corpo a partir de `body` na direcao `dir`
static Rectangle MeleeRect(Rectangle body, Vector2 dir, float reach, float thick) {
	Vector2 c = RectCenter(body);
	if (fabsf(dir.x) > fabsf(dir.y)) {
		float x = dir.x > 0 ? body.x + body.width : body.x - reach;
		return (Rectangle){x, c.y - thick / 2, reach, thick};
	}
	float y = dir.y > 0 ? body.y + body.height : body.y - reach;
	return (Rectangle){c.x - thick / 2, y, thick, reach};
}

static void SpawnProjectile(CombatState* c, bool fromPlayer, Vector2 center, Vector2 dir, float speed, float size, int dmg) {
	for (int i = 0; i < MAX_PROJECTILES; i++) {
		Projectile* p = &c->projectiles[i];
		if (p->active) { continue; }
		p->active = true;
		p->fromPlayer = fromPlayer;
		p->pos = center;
		p->vel = (Vector2){dir.x * speed, dir.y * speed};
		p->size = size;
		p->life = 2.5f;
		p->dmg = dmg;
		return;
	}
}

static void HurtPlayer(CombatState* c, int* plrHp, int dmg) {
	if (c->invuln > 0 || *plrHp <= 0) { return; }
	*plrHp -= dmg;
	if (*plrHp < 0) { *plrHp = 0; }
	c->invuln = PLAYER_INVULN;
}

static void HurtEnemy(Enemy* e, int dmg, Vector2 knockDir, float knockForce) {
	if (!e->present || !e->alive) { return; }
	e->hp -= dmg;
	e->hitFlash = 0.12f;
	e->knockback = (Vector2){knockDir.x * knockForce, knockDir.y * knockForce};
	if (e->hp <= 0) { e->hp = 0; e->alive = false; }
}

// move o inimigo respeitando paredes e limites do mundo
static void EnemyMove(Enemy* e, Vector2 delta, Vector2 worldSize, LoadedAssetsClass* Assets) {
	Rectangle next = EnemyBody(e);

	next.x = fminf(fmaxf(e->pos.x + delta.x, 0), worldSize.x - ENEMY_SIZE);
	if (!Assets->CheckColisions(Assets, next)) { e->pos.x = next.x; }

	next = EnemyBody(e);
	next.y = fminf(fmaxf(e->pos.y + delta.y, 0), worldSize.y - ENEMY_SIZE);
	if (!Assets->CheckColisions(Assets, next)) { e->pos.y = next.y; }
}

void Combat_Init(CombatState* c) {
	memset(c, 0, sizeof(CombatState));
	c->facing = (Vector2){0, 1};
}

void Combat_ClearEnemy(CombatState* c) {
	memset(&c->enemy, 0, sizeof(Enemy));
	memset(c->projectiles, 0, sizeof(c->projectiles));
	memset(c->slashes, 0, sizeof(c->slashes));
}

void Combat_SpawnEnemy(CombatState* c, Vector2 pos) {
	Combat_ClearEnemy(c);
	c->invuln = 0;
	c->enemy.present = true;
	c->enemy.alive = true;
	c->enemy.pos = pos;
	c->enemy.hp = c->enemy.maxHp = ENEMY_MAX_HP;
	c->enemy.state = ENEMY_CHASE;
	c->enemy.attackCd = 1.5f;   // da um tempinho antes do primeiro ataque
	c->enemy.facingRight = false;
}

void Combat_Update(CombatState* c, float dt, Rectangle plrBody, int* plrHp, Vector2 worldSize, LoadedAssetsClass* Assets, bool allowInput) {
	Enemy* e = &c->enemy;
	Rectangle plrHurt = PlayerHurtbox(plrBody);
	Vector2 plrCenter = RectCenter(plrBody);

	c->time += dt;
	c->lastPlayerBody = plrBody;
	if (c->meleeCd > 0) { c->meleeCd -= dt; }
	if (c->shootCd > 0) { c->shootCd -= dt; }
	if (c->invuln > 0)  { c->invuln -= dt; }

	// direcao em que o jogador olha
	if (IsKeyPressed(KEY_UP)    || IsKeyPressed(KEY_W)) { c->facing = (Vector2){0, -1}; }
	if (IsKeyPressed(KEY_DOWN)  || IsKeyPressed(KEY_S)) { c->facing = (Vector2){0, 1}; }
	if (IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A)) { c->facing = (Vector2){-1, 0}; }
	if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) { c->facing = (Vector2){1, 0}; }

	// ataques do jogador
	if (allowInput && *plrHp > 0) {
		if (IsKeyPressed(KEY_J) && c->meleeCd <= 0) {
			c->meleeCd = PLAYER_MELEE_CD;
			for (int i = 0; i < MAX_SLASHES; i++) {
				if (c->slashes[i].active) { continue; }
				c->slashes[i] = (Slash){true, c->facing, 0.15f, false, PLAYER_MELEE_DMG};
				break;
			}
		}

		if (IsKeyPressed(KEY_K) && c->shootCd <= 0) {
			c->shootCd = PLAYER_SHOT_CD;
			SpawnProjectile(c, true, plrCenter, c->facing, PLAYER_SHOT_SPEED, 14, PLAYER_SHOT_DMG);
		}
	}

	// golpes do jogador
	for (int i = 0; i < MAX_SLASHES; i++) {
		Slash* s = &c->slashes[i];
		if (!s->active) { continue; }

		s->life -= dt;
		if (s->life <= 0) { s->active = false; continue; }

		Rectangle area = MeleeRect(plrBody, s->dir, PLAYER_MELEE_REACH, PLAYER_MELEE_WIDTH);
		if (!s->hitDone && e->present && e->alive && CheckCollisionRecs(area, EnemyBody(e))) {
			s->hitDone = true;
			HurtEnemy(e, s->dmg, s->dir, 520.0f);
		}
	}

	// projeteis
	for (int i = 0; i < MAX_PROJECTILES; i++) {
		Projectile* p = &c->projectiles[i];
		if (!p->active) { continue; }

		p->pos.x += p->vel.x * dt;
		p->pos.y += p->vel.y * dt;
		p->life -= dt;

		Rectangle r = {p->pos.x - p->size / 2, p->pos.y - p->size / 2, p->size, p->size};

		if (p->life <= 0 || p->pos.x < -50 || p->pos.y < -50 || p->pos.x > worldSize.x + 50 || p->pos.y > worldSize.y + 50 || Assets->CheckColisions(Assets, r)) {
			p->active = false;
			continue;
		}

		if (p->fromPlayer) {
			if (e->present && e->alive && CheckCollisionRecs(r, EnemyBody(e))) {
				HurtEnemy(e, p->dmg, Norm(p->vel), 220.0f);
				p->active = false;
			}
		} else if (CheckCollisionRecs(r, plrHurt)) {
			HurtPlayer(c, plrHp, p->dmg);
			p->active = false;
		}
	}

	// IA do inimigo
	if (!e->present || !e->alive) { return; }

	if (e->hitFlash > 0) { e->hitFlash -= dt; }
	if (e->attackCd > 0) { e->attackCd -= dt; }

	// knockback decai rapido
	EnemyMove(e, (Vector2){e->knockback.x * dt, e->knockback.y * dt}, worldSize, Assets);
	float decay = fmaxf(0.0f, 1.0f - 9.0f * dt);
	e->knockback.x *= decay;
	e->knockback.y *= decay;

	Vector2 eCenter = RectCenter(EnemyBody(e));
	Vector2 toPlr = {plrCenter.x - eCenter.x, plrCenter.y - eCenter.y};
	float dist = sqrtf(toPlr.x * toPlr.x + toPlr.y * toPlr.y);
	Vector2 dir = Norm(toPlr);
	bool plrAlive = *plrHp > 0;

	switch (e->state) {
	case ENEMY_CHASE:
		if (fabsf(dir.x) > 0.1f) { e->facingRight = dir.x > 0; }
		if (!plrAlive) { break; }

		if (dist > 70.0f) {
			EnemyMove(e, (Vector2){dir.x * ENEMY_SPEED * dt, dir.y * ENEMY_SPEED * dt}, worldSize, Assets);
		}

		if (e->attackCd <= 0) {
			if (dist < 120.0f) {
				e->state = ENEMY_MELEE_WINDUP;
				e->stateTimer = 0.55f;
				e->strikeDir = dir;
			} else if (dist > 230.0f) {
				e->state = ENEMY_SHOOT_WINDUP;
				e->stateTimer = 0.5f;
			}
		}
		break;

	case ENEMY_MELEE_WINDUP:   // telegrafa o golpe: da tempo de fugir
		e->stateTimer -= dt;
		if (e->stateTimer <= 0) {
			e->state = ENEMY_MELEE_STRIKE;
			e->stateTimer = 0.15f;
			e->strikeHit = false;
		}
		break;

	case ENEMY_MELEE_STRIKE: {
		e->stateTimer -= dt;
		Rectangle area = MeleeRect(EnemyBody(e), e->strikeDir, 80.0f, 90.0f);
		if (!e->strikeHit && CheckCollisionRecs(area, plrHurt) && c->invuln <= 0) {
			e->strikeHit = true;
			HurtPlayer(c, plrHp, ENEMY_MELEE_DMG);
		}
		if (e->stateTimer <= 0) {
			e->state = ENEMY_RECOVER;
			e->stateTimer = 0.5f;
			e->attackCd = 1.0f;
		}
		break;
	}

	case ENEMY_SHOOT_WINDUP:
		if (fabsf(dir.x) > 0.1f) { e->facingRight = dir.x > 0; }
		e->stateTimer -= dt;
		if (e->stateTimer <= 0) {
			SpawnProjectile(c, false, eCenter, dir, ENEMY_SHOT_SPEED, 18, ENEMY_SHOT_DMG);

			// com pouca vida ele fica furioso e atira em leque
			if (e->hp < e->maxHp / 2) {
				float ang = atan2f(dir.y, dir.x);
				SpawnProjectile(c, false, eCenter, (Vector2){cosf(ang + 0.3f), sinf(ang + 0.3f)}, ENEMY_SHOT_SPEED, 18, ENEMY_SHOT_DMG);
				SpawnProjectile(c, false, eCenter, (Vector2){cosf(ang - 0.3f), sinf(ang - 0.3f)}, ENEMY_SHOT_SPEED, 18, ENEMY_SHOT_DMG);
			}

			e->state = ENEMY_RECOVER;
			e->stateTimer = 0.4f;
			e->attackCd = 1.2f;
		}
		break;

	case ENEMY_RECOVER:
		e->stateTimer -= dt;
		if (e->stateTimer <= 0) { e->state = ENEMY_CHASE; }
		break;
	}
}

// ---------------------------------------------------------------- desenho
static Color SpriteColor(char ch, const Enemy* e) {
	bool angry = e->hp < e->maxHp / 2;
	switch (ch) {
		case 'H': return (Color){240, 220, 140, 255};
		case 'P': return angry ? (Color){170, 60, 110, 255} : (Color){130, 70, 190, 255};
		case 'D': return angry ? (Color){110, 30, 70, 255}  : (Color){80, 40, 130, 255};
		case 'W': return WHITE;
		case 'E': return (e->state == ENEMY_MELEE_WINDUP || e->state == ENEMY_SHOOT_WINDUP) ? RED : BLACK;
		case 'M': return (Color){90, 10, 30, 255};
		case 'F': return (Color){40, 20, 60, 255};
		default:  return BLANK;
	}
}

static void DrawEnemySprite(const Enemy* e) {
	bool flash = e->hitFlash > 0;

	for (int row = 0; row < ENEMY_GRID; row++) {
		for (int col = 0; col < ENEMY_GRID; col++) {
			int srcCol = e->facingRight ? (ENEMY_GRID - 1 - col) : col;
			char ch = ENEMY_SPRITE[row][srcCol];
			if (ch == '.') { continue; }

			Color color = flash ? WHITE : SpriteColor(ch, e);
			DrawRectangle((int)e->pos.x + col * ENEMY_PIXEL, (int)e->pos.y + row * ENEMY_PIXEL, ENEMY_PIXEL, ENEMY_PIXEL, color);
		}
	}
}

void Combat_Draw(CombatState* c) {
	Enemy* e = &c->enemy;

	if (e->present && e->alive) {
		// sombra
		DrawEllipse((int)(e->pos.x + ENEMY_SIZE / 2), (int)(e->pos.y + ENEMY_SIZE), ENEMY_SIZE / 2, 8, (Color){0, 0, 0, 90});

		// aviso do golpe corpo a corpo
		if (e->state == ENEMY_MELEE_WINDUP) {
			Rectangle area = MeleeRect(EnemyBody(e), e->strikeDir, 80.0f, 90.0f);
			float pulse = 0.25f + 0.2f * sinf(c->time * 30.0f);
			DrawRectangleRec(area, Fade(RED, pulse));
			DrawRectangleLinesEx(area, 2, Fade(RED, 0.8f));
		}
		if (e->state == ENEMY_MELEE_STRIKE) {
			DrawRectangleRec(MeleeRect(EnemyBody(e), e->strikeDir, 80.0f, 90.0f), Fade(RED, 0.75f));
		}
		if (e->state == ENEMY_SHOOT_WINDUP) {
			DrawText("!", (int)(e->pos.x + ENEMY_SIZE / 2 - 6), (int)e->pos.y - 34, 32, RED);
		}

		DrawEnemySprite(e);
	}

	// golpes do jogador: precisa da posicao do jogador -> guardamos o rect no proprio draw
	// (recalculado a partir do ultimo `plrBody`, ver Combat_Update)
	for (int i = 0; i < MAX_SLASHES; i++) {
		Slash* s = &c->slashes[i];
		if (!s->active) { continue; }
		Rectangle area = MeleeRect(c->lastPlayerBody, s->dir, PLAYER_MELEE_REACH, PLAYER_MELEE_WIDTH);
		DrawRectangleRec(area, Fade(WHITE, s->life / 0.15f * 0.7f));
		DrawRectangleLinesEx(area, 2, SKYBLUE);
	}

	for (int i = 0; i < MAX_PROJECTILES; i++) {
		Projectile* p = &c->projectiles[i];
		if (!p->active) { continue; }
		Rectangle r = {p->pos.x - p->size / 2, p->pos.y - p->size / 2, p->size, p->size};
		if (p->fromPlayer) {
			DrawRectangleRec(r, SKYBLUE);
			DrawRectangle((int)r.x + 4, (int)r.y + 4, (int)r.width - 8, (int)r.height - 8, WHITE);
		} else {
			DrawRectangleRec(r, (Color){200, 30, 60, 255});
			DrawRectangle((int)r.x + 5, (int)r.y + 5, (int)r.width - 10, (int)r.height - 10, ORANGE);
		}
	}
}

static void DrawBar(Vector2 pos, Vector2 size, float alpha, Color color, char *text) {
	DrawRectangle(pos.x-3, pos.y-3, size.x+6, size.y+6, BLACK);
	DrawRectangleV(pos, size, (Color){60, 60, 60, 255});
	DrawRectangleV(pos, (Vector2){size.x * alpha, size.y}, color);
	DrawText(text, pos.x+8, pos.y+4, size.y-4, WHITE);
}

void Combat_DrawHud(CombatState* c, int plrHp) {
	Enemy* e = &c->enemy;
	int sw = GetScreenWidth(), sh = GetScreenHeight();

	// vida do jogador
	DrawBar((Vector2){20, sh-50}, (Vector2){260, 22}, (float)plrHp / PLAYER_MAX_HP, plrHp > 30 ? GREEN : ORANGE, TextFormat("HP %d/%d", plrHp < 0 ? 0 : plrHp, PLAYER_MAX_HP));

	// vida do inimigo
	if (e->present) {
		float w = 420;
		DrawBar((Vector2){sw / 2 - w / 2, 20}, (Vector2){w, 20}, (float)e->hp / e->maxHp, RED, "");
		DrawText("DEMONINHO", sw / 2 - MeasureText("DEMONINHO", 18) / 2, 44, 18, WHITE);
	}

	DrawText("[J] Golpe   [K] Tiro   [R] Reiniciar", sw - 360, sh - 30, 18, LIGHTGRAY);

	if (plrHp <= 0) {
		DrawRectangle(0, sh / 2 - 60, sw, 120, (Color){0, 0, 0, 180});
		const char* msg = "perdeu aperta [R] para reiniciar";
		DrawText(msg, sw / 2 - MeasureText(msg, 40) / 2, sh / 2 - 20, 40, RED);
	} else if (e->present && !e->alive) {
		DrawRectangle(0, sh / 2 - 60, sw, 120, (Color){0, 0, 0, 180});
		const char* msg = "venceu aperta [R] para reiniciar";
		DrawText(msg, sw / 2 - MeasureText(msg, 40) / 2, sh / 2 - 20, 40, GOLD);
	}
}

bool Combat_PlayerBlink(CombatState* c) {
	return c->invuln > 0 && ((int)(c->time * 20.0f) % 2 == 0);
}
