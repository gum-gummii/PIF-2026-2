#include <progression.h>
#include <stdio.h>
#include <string.h>

#define ENCOUNTER_GRACE  1.5f
#define BANNER_TIME      3.0f

static void ShowBanner(Progression* p, const char* msg) {
	snprintf(p->banner, sizeof(p->banner), "%s", msg);
	p->bannerT = BANNER_TIME;
}

void Progression_Reset(Progression* p) {
	memset(p, 0, sizeof(*p));
	p->fightingMinion = -1;
}

bool Progression_CanEncounter(const Progression* p) {
	return p->fightingDef == NULL && p->grace <= 0;
}

int Progression_WinsLeft(const Progression* p) {
	int left = WINS_TO_OPEN_CASINO - p->wins;
	return left > 0 ? left : 0;
}

void Progression_BeginFight(Progression* p, int minionIndex, const EnemyDef* def) {
	p->fightingMinion = minionIndex;
	p->fightingDef = def;
}

static void OnVictory(Progression* p, const EnemyDef* def) {
	if (def->isBoss) {
		p->cleared = true;
		ShowBanner(p, "VOCE VENCEU O CHEFE! FASE LIMPA");
		return;
	}

	if (def->dropsKey && !p->hasKey) {
		p->hasKey = true;
		ShowBanner(p, "Voce pegou a CHAVE!");
	}

	p->wins++;
	if (!p->casinoUnlocked && p->wins >= WINS_TO_OPEN_CASINO) {
		p->casinoUnlocked = true;
		ShowBanner(p, "A porta do KARD KASSINO destrancou!");
	}
}

void Progression_EndFight(Progression* p, BattleOutcome outcome, StageContext ctx) {
	int index = p->fightingMinion;
	const EnemyDef* def = p->fightingDef;
	p->fightingMinion = -1;
	p->fightingDef = NULL;
	p->grace = ENCOUNTER_GRACE;

	if (def == NULL || outcome == BATTLE_ONGOING || outcome == BATTLE_LOST) { return; }

	bool keepMinion = outcome == BATTLE_FLED && Minions_IsPermanent(ctx.minions, index);
	if (index >= 0 && !keepMinion) {
		Vector2 pos = Minions_Position(ctx.minions, index);
		Minions_Remove(ctx.minions, index);
		if (outcome != BATTLE_FLED) { Score_AddKill(ctx.score, def->points, (Vector2){pos.x, pos.y - 40}); }
	}

	if (outcome == BATTLE_FLED) { return; }
	OnVictory(p, def);
}

void Progression_SetInBossRoom(Progression* p, bool inBossRoom) {
	p->inBossRoom = inBossRoom;
}

void Progression_Update(Progression* p, float dt) {
	if (p->grace > 0) { p->grace -= dt; }
	if (p->bannerT > 0) { p->bannerT -= dt; }
}

static const char* ObjectiveText(const Progression* p, Color* color) {
	*color = WHITE;
	if (p->cleared) { *color = GOLD; return "Fase limpa!"; }
	if (p->inBossRoom) { *color = (Color){255, 120, 120, 255}; return "CHEFE: derrote o DRAGAO"; }
	if (p->hasKey) { *color = (Color){120, 255, 200, 255}; return "Use a CHAVE na sala 2"; }
	if (p->casinoUnlocked) { *color = (Color){120, 255, 200, 255}; return "Ache a CHAVE dentro do KARD KASSINO"; }
	return TextFormat("Inimigos vencidos: %d/%d", p->wins, WINS_TO_OPEN_CASINO);
}

static void DrawObjective(const Progression* p) {
	Color color;
	const char* txt = ObjectiveText(p, &color);

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
