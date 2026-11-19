#include <progression.h>
#include <stdio.h>
#include <string.h>

#define ENCOUNTER_GRACE  1.5f
#define BANNER_TIME      3.0f

static void ShowBanner(Progression* p, const char* msg) {
	snprintf(p->banner, sizeof(p->banner), "%s", msg);
	p->bannerT = BANNER_TIME;
}

void Progression_Reset(Progression* p, int winsToUnlock) {
	memset(p, 0, sizeof(*p));
	p->winsToUnlock = winsToUnlock;
	p->fighting = -1;
}

bool Progression_CanEncounter(const Progression* p) {
	return p->fighting < 0 && p->grace <= 0;
}

int Progression_WinsLeft(const Progression* p) {
	int left = p->winsToUnlock - p->wins;
	return left > 0 ? left : 0;
}

void Progression_BeginFight(Progression* p, int minionIndex) {
	p->fighting = minionIndex;
}

static void OnVictory(Progression* p, const EnemyDef* def) {
	if (def->isBoss) {
		p->cleared = true;
		ShowBanner(p, "VOCE VENCEU O CHEFE! FASE LIMPA");
		return;
	}

	p->wins++;
	if (p->winsToUnlock > 0 && !p->doorUnlocked && p->wins >= p->winsToUnlock) {
		p->doorUnlocked = true;
		ShowBanner(p, "A porta do KARD KASSINO destrancou!");
	}
}

void Progression_EndFight(Progression* p, BattleOutcome outcome, StageContext ctx) {
	int index = p->fighting;
	p->fighting = -1;
	p->grace = ENCOUNTER_GRACE;

	if (index < 0 || outcome == BATTLE_ONGOING || outcome == BATTLE_LOST) { return; }

	const EnemyDef* def = Enemy_Get(Minions_EnemyOf(ctx.minions, index));
	Vector2 pos = Minions_Position(ctx.minions, index);
	Minions_Remove(ctx.minions, index);

	if (outcome == BATTLE_FLED) { return; }

	Score_AddKill(ctx.score, def->points, (Vector2){pos.x, pos.y - 40});
	OnVictory(p, def);
}

void Progression_SetInBossRoom(Progression* p, bool inBossRoom) {
	if (inBossRoom && !p->inBossRoom && !p->cleared) { ShowBanner(p, "SALA DO CHEFE"); }
	p->inBossRoom = inBossRoom;
}

void Progression_Update(Progression* p, float dt) {
	if (p->grace > 0) { p->grace -= dt; }
	if (p->bannerT > 0) { p->bannerT -= dt; }
}

static const char* ObjectiveText(const Progression* p, Color* color) {
	*color = WHITE;
	if (p->cleared) { *color = GOLD; return "Fase limpa!"; }
	if (p->inBossRoom) { *color = (Color){255, 120, 120, 255}; return "CHEFE: derrote o DEMONINHO"; }
	if (p->winsToUnlock <= 0) { return NULL; }
	if (p->doorUnlocked) { *color = (Color){120, 255, 200, 255}; return "Entre no KARD KASSINO"; }
	return TextFormat("Inimigos vencidos: %d/%d", p->wins, p->winsToUnlock);
}

static void DrawObjective(const Progression* p) {
	Color color;
	const char* txt = ObjectiveText(p, &color);
	if (txt == NULL) { return; }

	int fs = 24;
	int x = 15, y = GetScreenHeight() - 75;
	DrawRectangle(x - 8, y - 6, MeasureText(txt, fs) + 16, fs + 12, Fade(BLACK, 0.55f));
	DrawText(txt, x, y, fs, color);
}

static void DrawBanner(const Progression* p) {
	if (p->bannerT <= 0) { return; }

	float alpha = p->bannerT < 0.5f ? p->bannerT / 0.5f : 1.0f;
	int fs = 40;
	int w = MeasureText(p->banner, fs);
	int x = GetScreenWidth() / 2 - w / 2, y = 140;
	DrawRectangle(x - 20, y - 12, w + 40, fs + 24, Fade(BLACK, 0.7f * alpha));
	DrawText(p->banner, x, y, fs, Fade(GOLD, alpha));
}

void Progression_DrawHud(const Progression* p) {
	DrawObjective(p);
	DrawBanner(p);
}
