#include <progression.h>
#include <stdio.h>
#include <string.h>

#define ENCOUNTER_GRACE  1.5f
#define BANNER_TIME      3.0f

static void ShowBanner(Progression* p, const char* msg) {
	snprintf(p->banner, sizeof(p->banner), "%s", msg);
	p->bannerT = BANNER_TIME;
}

void Progression_Reset(Progression* p, int winsToBoss) {
	memset(p, 0, sizeof(*p));
	p->winsToBoss = winsToBoss;
	p->fighting = -1;
}

bool Progression_CanEncounter(const Progression* p) {
	return p->fighting < 0 && p->grace <= 0;
}

void Progression_BeginFight(Progression* p, int minionIndex) {
	p->fighting = minionIndex;
}

static void OnVictory(Progression* p, const EnemyDef* def) {
	if (def->isBoss) {
		p->bossOnMap = false;
		p->cleared = true;
		ShowBanner(p, "FASE LIMPA! Aperte [E] para seguir");
		return;
	}

	p->wins++;
	bool unlocked = p->winsToBoss > 0 && p->wins >= p->winsToBoss;
	if (unlocked && !p->bossOnMap && !p->cleared) { p->bossPending = true; }
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

void Progression_Update(Progression* p, float dt, StageContext ctx) {
	if (p->grace > 0) { p->grace -= dt; }
	if (p->bannerT > 0) { p->bannerT -= dt; }

	if (!p->bossPending) { return; }
	if (Minions_SpawnBoss(ctx.minions, ctx.worldSize, ctx.plrBody, ctx.Assets) < 0) { return; }

	p->bossPending = false;
	p->bossOnMap = true;
	ShowBanner(p, "O DEMONINHO apareceu no mapa!");
}

static void DrawObjective(const Progression* p) {
	if (p->winsToBoss <= 0) { return; }

	const char* txt;
	Color color = WHITE;
	if (p->cleared) { txt = "Fase limpa!"; color = GOLD; }
	else if (p->bossOnMap) { txt = "CHEFE: encontre o DEMONINHO"; color = (Color){255, 120, 120, 255}; }
	else { txt = TextFormat("Inimigos vencidos: %d/%d", p->wins, p->winsToBoss); }

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
