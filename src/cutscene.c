#include <cutscene.h>
#include <math.h>
#include <string.h>

#define TEXT_CPS     38.0f
#define LAUGH_TIME   2.4f
#define EMERGE_TIME  1.0f

typedef enum { A_NARA, A_IRMA, A_GAMBIT, A_BILHETE, A_COUNT } ActorId;
typedef enum { P_IDLE, P_WALK, P_HIDDEN, P_HAPPY, P_GIGGLE, P_LAUGH, P_DEAD, P_EMERGE, P_ROAR } Pose;
typedef enum { CS_SHOW, CS_HIDE, CS_WALK, CS_POSE, CS_WIGGLE, CS_HOP, CS_CAMERA, CS_SAY, CS_NOTE, CS_WAIT, CS_SHAKE, CS_FLASH, CS_FADE, CS_END } StepKind;
typedef enum { ART_SALA_BOSS, ART_JARDIM, ART_IRMA, ART_GAMBIT, ART_BILHETE, ART_COUNT } ArtId;

typedef struct {
	StepKind kind;
	ActorId who;
	float x, y, time;
	Pose pose;
	const char* speaker;
	const char* text;
	bool together;
} Step;

typedef struct { float from, to, t, dur; } Tween;
typedef struct { const char* file; float fw, fh; Color color; } ArtDef;

typedef struct {
	bool on;
	Vector2 pos;
	Tween x, y;
	Pose pose;
	float poseT, poseDur, wiggle, hop, h;
} Actor;

static const ArtDef ARTS[ART_COUNT] = {
	[ART_SALA_BOSS] = {"karddkassino_bossroom.png"},
	[ART_JARDIM]    = {"kardkassino_garden.png"},
	[ART_IRMA]      = {"gambitcutscene.png", 120, 150, {150, 210, 90, 255}},
	[ART_GAMBIT]    = {"Gambit.png", 300, 300, {235, 235, 235, 255}},
	[ART_BILHETE]   = {"bilhete.png", 0, 0, {245, 235, 200, 255}},
};

static const Step INTRO_SCRIPT[] = {
	{CS_SHOW, A_NARA, 520, 375, .pose = P_IDLE},
	{CS_SHOW, A_IRMA, 700, 375, .pose = P_HAPPY},
	{CS_FADE, .x = 0, .time = 1.0f},
	{CS_HOP, A_NARA, .time = 2.6f, .together = true},
	{CS_HOP, A_IRMA, .time = 2.6f, .together = true},
	{CS_WALK, A_NARA, 600, 375, .time = 1.3f, .together = true},
	{CS_WALK, A_IRMA, 780, 375, .time = 1.3f},
	{CS_WALK, A_NARA, 500, 375, .time = 1.3f, .together = true},
	{CS_WALK, A_IRMA, 690, 375, .time = 1.3f},
	{CS_POSE, A_IRMA, .pose = P_HAPPY},
	{CS_WAIT, .time = 0.4f},
	{CS_FADE, .x = 1, .time = 0.25f},
	{.kind = CS_HIDE, .who = A_IRMA},
	{CS_SHOW, A_BILHETE, 690, 372, .pose = P_IDLE},
	{CS_WAIT, .time = 1.4f},
	{CS_FADE, .x = 0, .time = 0.8f},
	{CS_WIGGLE, A_NARA, .time = 0.7f},
	{CS_WAIT, .time = 0.6f},
	{CS_WALK, A_NARA, 640, 375, .time = 1.2f},
	{CS_WAIT, .time = 0.4f},
	{CS_NOTE, .text = "Come to the Kassino\nif you want to save her."},
	{CS_FADE, .x = 1, .time = 0.8f},
	{.kind = CS_END},
};

static const Step BOSS_SCRIPT[] = {
	// quadro 1
	{CS_SHOW, A_NARA, -80, 440, .pose = P_WALK},
	{CS_SHOW, A_IRMA, 1590, 255, .pose = P_HIDDEN},
	{CS_FADE, .x = 0, .time = 0.8f, .together = true},
	{CS_WALK, A_NARA, 430, 440, .time = 2.2f},
	{CS_SAY, .speaker = "Nara", .text = "Show yourself, tyrant!"},
	// quadro 2
	{CS_CAMERA, .x = 488, .time = 1.4f},
	{CS_WAIT, .time = 0.5f},
	{CS_WIGGLE, A_IRMA, .time = 0.9f},
	{CS_WAIT, .time = 0.3f},
	// quadro 3
	{CS_CAMERA, .x = 140, .time = 1.8f, .together = true},
	{CS_WALK, A_IRMA, 1120, 440, .time = 1.8f},
	{CS_POSE, A_IRMA, .pose = P_GIGGLE, .time = 1.2f},
	// quadro 4
	{CS_SAY, .speaker = "Nara", .text = "WHAT'S SO FUNNY, HUH?"},
	// quadro 5
	{CS_POSE, A_IRMA, .pose = P_IDLE, .time = 0.5f},
	{CS_SAY, .speaker = "Nara", .text = "S-sis..?"},
	{CS_SAY, .speaker = "Nara", .text = "What happened to you..?"},
	// quadro 6
	{CS_SHAKE, .x = 3, .time = LAUGH_TIME, .together = true},
	{CS_POSE, A_IRMA, .pose = P_LAUGH, .time = LAUGH_TIME},
	{CS_SHAKE, .x = 4, .time = 1.2f, .together = true},
	{CS_POSE, A_IRMA, .pose = P_DEAD, .time = 1.2f},
	{.kind = CS_FLASH},
	{.kind = CS_HIDE, .who = A_IRMA},
	{CS_SHAKE, .x = 6, .time = EMERGE_TIME, .together = true},
	{CS_SHOW, A_GAMBIT, 1150, 450, .pose = P_EMERGE},
	{CS_WAIT, .time = EMERGE_TIME},
	// quadro 7
	{CS_POSE, A_GAMBIT, .pose = P_ROAR},
	{CS_SHAKE, .x = 10, .time = 1.4f, .together = true},
	{CS_SAY, .text = "* Roars"},
	// quadro 8
	{CS_POSE, A_GAMBIT, .pose = P_IDLE},
	{CS_SAY, .speaker = "Nara", .text = "You will PAY FOR THAT!"},
	{CS_FADE, .x = 1, .time = 0.6f},
	{.kind = CS_END},
};

static struct {
	bool active, loaded;
	const Step* script;
	int count, step;
	ArtId background;
	float stepT, wait, time, lock;
	Actor a[A_COUNT];
	float cam, fade, shake, shakeAmp, flash;
	Tween camW, fadeW;
	const char* speaker;
	const char* text;
	bool note;
	float textT;
	Texture2D art[ART_COUNT];
	Texture2D nara;
	Rectangle frame;
} cs;

static float Clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
static float Ease(float t) { t = Clamp01(t); return t * t * (3 - 2 * t); }
static float Lerp(float a, float b, float t) { return a + (b - a) * t; }
static Tween To(float from, float to, float dur) { return (Tween){from, to, 0, dur}; }

static float Advance(Tween* w, float dt, float cur) {
	if (w->dur <= 0 || w->t >= w->dur) { return cur; }
	w->t += dt;
	return Lerp(w->from, w->to, Ease(w->t / w->dur));
}

static bool TextDone(void) { return !cs.text || (int)(cs.textT * TEXT_CPS) >= (int)strlen(cs.text); }

static float StartStep(const Step* s) {
	Actor* a = &cs.a[s->who];
	switch (s->kind) {
	case CS_SHOW:   *a = (Actor){true, {s->x, s->y}, .pose = s->pose, .h = a->h}; return 0;
	case CS_HIDE:   a->on = false; return 0;
	case CS_WALK:   a->x = To(a->pos.x, s->x, s->time); a->y = To(a->pos.y, s->y, s->time); if (a->pose != P_HAPPY) { a->pose = P_WALK; } return s->time;
	case CS_POSE:   a->pose = s->pose; a->poseT = 0; a->poseDur = s->time; return s->time;
	case CS_WIGGLE: a->wiggle = s->time; return s->time;
	case CS_HOP:    a->hop = s->time; return s->time;
	case CS_CAMERA: cs.camW = To(cs.cam, s->x, s->time); return s->time;
	case CS_SAY:
	case CS_NOTE:   cs.speaker = s->speaker; cs.text = s->text; cs.textT = 0; cs.note = s->kind == CS_NOTE; return -1;
	case CS_WAIT:   return s->time;
	case CS_SHAKE:  cs.shake = s->time; cs.shakeAmp = s->x; return s->time;
	case CS_FLASH:  cs.flash = 0.35f; return 0;
	case CS_FADE:   cs.fadeW = To(cs.fade, s->x, s->time); return s->time;
	default:        cs.active = false; return 0;
	}
}

static bool Waits(const Step* s) { return s->kind == CS_SAY || s->kind == CS_NOTE; }

static void RunSteps(void) {
	for (; cs.active && cs.step < cs.count; cs.step++) {
		const Step* s = &cs.script[cs.step];
		cs.stepT = 0;
		cs.wait = StartStep(s);
		if (!s->together && (cs.wait != 0 || Waits(s))) { return; }
	}
	cs.active = false;
}

static void Start(const Step* script, int count, ArtId background, Texture2D naraSheet, Rectangle naraFrame) {
	if (!cs.loaded) {
		for (int i = 0; i < ART_COUNT; i++) {
			cs.art[i] = FileExists(ARTS[i].file) ? LoadTexture(ARTS[i].file) : (Texture2D){0};
		}
		cs.loaded = true;
	}
	memset(cs.a, 0, sizeof(cs.a));
	cs.a[A_NARA].h = 150;
	cs.a[A_IRMA].h = 150;
	cs.a[A_GAMBIT].h = 360;
	cs.a[A_BILHETE].h = 16;
	cs.script = script;
	cs.count = count;
	cs.background = background;
	cs.active = true;
	cs.step = 0;
	cs.time = cs.cam = cs.shake = cs.flash = 0;
	cs.camW = cs.fadeW = (Tween){0};
	cs.fade = 1;
	cs.lock = 0.3f;
	cs.text = cs.speaker = NULL;
	cs.nara = naraSheet;
	cs.frame = naraFrame;
	RunSteps();
}

#define SCRIPT_LEN(s) ((int)(sizeof(s) / sizeof((s)[0])))
void Cutscene_StartIntro(Texture2D naraSheet, Rectangle naraFrame) { Start(INTRO_SCRIPT, SCRIPT_LEN(INTRO_SCRIPT), ART_JARDIM, naraSheet, naraFrame); }
void Cutscene_StartBoss(Texture2D naraSheet, Rectangle naraFrame) { Start(BOSS_SCRIPT, SCRIPT_LEN(BOSS_SCRIPT), ART_SALA_BOSS, naraSheet, naraFrame); }
void Cutscene_Stop(void) { cs.active = false; }
bool Cutscene_IsActive(void) { return cs.active; }

void Cutscene_Unload(void) {
	for (int i = 0; i < ART_COUNT; i++) { if (cs.art[i].id != 0) { UnloadTexture(cs.art[i]); } }
	cs.loaded = false;
}

void Cutscene_Update(float dt) {
	if (!cs.active) { return; }
	cs.time += dt;
	cs.textT += dt;
	cs.lock -= dt;
	cs.shake -= dt;
	cs.flash -= dt;
	cs.cam = Advance(&cs.camW, dt, cs.cam);
	cs.fade = Advance(&cs.fadeW, dt, cs.fade);
	for (int i = 0; i < A_COUNT; i++) {
		Actor* a = &cs.a[i];
		a->poseT += dt;
		a->wiggle -= dt;
		a->hop -= dt;
		a->pos = (Vector2){Advance(&a->x, dt, a->pos.x), Advance(&a->y, dt, a->pos.y)};
		if (a->pose == P_WALK && a->x.t >= a->x.dur) { a->pose = P_IDLE; }
	}

	bool ok = cs.lock <= 0 && (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_ENTER));
	if (cs.lock <= 0 && IsKeyPressed(KEY_X)) { cs.active = false; return; }

	if (Waits(&cs.script[cs.step])) {
		if (!ok) { return; }
		if (!TextDone()) { cs.textT = 999; return; }
		cs.text = cs.speaker = NULL;
	} else if ((cs.stepT += dt) < cs.wait) {
		return;
	}
	cs.step++;
	RunSteps();
}

static void DrawSheet(ArtId id, Texture2D tex, int col, int row, Vector2 feet, float h, Color tint) {
	const ArtDef* d = &ARTS[id];
	float fw = d->fw > 0 ? d->fw : (tex.id ? (float)tex.width : h);
	float fh = d->fh > 0 ? d->fh : (tex.id ? (float)tex.height : h);
	float w = fw * h / fh;
	Rectangle dst = {feet.x - w / 2, feet.y - h, w, h};
	if (tex.id) {
		DrawTexturePro(tex, (Rectangle){fw * col, fh * row, fw, fh}, dst, (Vector2){0, 0}, 0, tint);
		return;
	}
	DrawRectangleRec(dst, ColorTint(d->color, tint));
	DrawRectangleLinesEx(dst, 2, BLACK);
	if (h > 30) {
		const char* tag = TextFormat("[%s]", d->file);
		DrawText(tag, (int)feet.x - MeasureText(tag, 14) / 2, (int)feet.y + 6, 14, Fade(LIGHTGRAY, 0.8f));
	}
}

static void DrawLaugh(const char* word, Vector2 head, float k) {
	for (int i = 0; i < 3; i++) {
		float p = fmodf(cs.time * (1.2f + k) + i / 3.0f, 1.0f);
		int fs = (int)(20 + k * 16);
		DrawText(word, (int)(head.x + (i % 2 ? 1 : -1) * (40 + p * 30)), (int)(head.y - 30 - p * 60), fs, Fade(WHITE, 1 - p));
	}
}

static void DrawActor(ActorId id, const Actor* a, Vector2 feet) {
	int anim = (int)(cs.time * 8);
	float k = a->poseDur > 0 ? Clamp01(a->poseT / a->poseDur) : 1;
	Vector2 head = {feet.x, feet.y - a->h};

	switch (id) {
	case A_NARA: {
		Rectangle f = cs.frame;
		bool walk = a->pose == P_WALK;
		float row = a->x.to < a->x.from ? 2 : 4;
		Rectangle src = {f.width * (walk ? anim % 4 : 3), walk ? f.height * row : 0, f.width, f.height};
		float w = f.width * a->h / f.height;
		DrawTexturePro(cs.nara, src, (Rectangle){feet.x - w / 2, feet.y - a->h, w, a->h}, (Vector2){0, 0}, 0, WHITE);
		break;
	}
	case A_IRMA: {
		Texture2D t = cs.art[ART_IRMA];
		switch (a->pose) {
		case P_HIDDEN: DrawSheet(ART_IRMA, t, 0, 0, feet, a->h, (Color){30, 25, 45, 255}); break;
		case P_HAPPY:  DrawSheet(ART_IRMA, t, 0, 1, feet, a->h, WHITE); break;
		case P_GIGGLE: DrawSheet(ART_IRMA, t, anim / 3 % 2, 1, feet, a->h, WHITE); DrawLaugh("HE", head, 0.2f); break;
		case P_LAUGH:  DrawSheet(ART_IRMA, t, 1 + (int)(k * 2.99f), 1, feet, a->h, WHITE); DrawLaugh("HA", head, k); break;
		case P_DEAD:   DrawSheet(ART_IRMA, t, k < 0.5f ? 0 : 1, 2, feet, a->h, WHITE); break;
		default:       DrawSheet(ART_IRMA, t, 3, 0, feet, a->h, WHITE); break;
		}
		break;
	}
	case A_GAMBIT: {
		Texture2D t = cs.art[ART_GAMBIT];
		float grow = a->pose == P_EMERGE ? 0.75f + 0.25f * Ease(a->poseT / EMERGE_TIME) : 1.0f;
		if (a->pose == P_EMERGE)    { DrawSheet(ART_GAMBIT, t, 1, 0, feet, a->h * grow, WHITE); }
		else if (a->pose == P_ROAR) { DrawSheet(ART_GAMBIT, t, 0, 0, feet, a->h, WHITE); }
		else                        { DrawSheet(ART_GAMBIT, t, anim / 2 % 4, 1, feet, a->h, WHITE); }
		break;
	}
	case A_BILHETE:
		if (cs.art[ART_BILHETE].id) { DrawSheet(ART_BILHETE, cs.art[ART_BILHETE], 0, 0, feet, 26, WHITE); break; }
		DrawRectanglePro((Rectangle){feet.x, feet.y - 8, 28, 18}, (Vector2){14, 9}, -12, (Color){245, 235, 200, 255});
		DrawLineEx((Vector2){feet.x - 9, feet.y - 9}, (Vector2){feet.x + 8, feet.y - 12}, 2, GRAY);
		DrawLineEx((Vector2){feet.x - 8, feet.y - 4}, (Vector2){feet.x + 9, feet.y - 7}, 2, GRAY);
		break;
	default:
		break;
	}
}

static void DrawNote(void) {
	int sw = GetScreenWidth(), sh = GetScreenHeight();
	DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.6f));
	Rectangle paper = {sw / 2.0f - 300, sh / 2.0f - 150, 600, 300};
	DrawRectangleRec(paper, (Color){245, 235, 200, 255});
	DrawRectangleLinesEx(paper, 3, (Color){150, 120, 80, 255});
	int n = (int)(cs.textT * TEXT_CPS);
	DrawText(TextSubtext(cs.text, 0, n), (int)paper.x + 50, (int)paper.y + 90, 40, (Color){80, 40, 30, 255});
}

static void DrawDialogue(void) {
	if (!cs.text) { return; }
	if (cs.note) {
		DrawNote();
	} else {
		Rectangle box = {60, (float)GetScreenHeight() - 200, (float)GetScreenWidth() - 120, 160};
		DrawRectangleRec(box, Fade(BLACK, 0.92f));
		DrawRectangleLinesEx(box, 5, WHITE);
		DrawRectangleLinesEx((Rectangle){box.x + 5, box.y + 5, box.width - 10, box.height - 10}, 2, (Color){160, 60, 220, 255});
		if (cs.speaker) { DrawText(cs.speaker, (int)box.x + 30, (int)box.y + 18, 26, YELLOW); }
		DrawText(TextSubtext(cs.text, 0, (int)(cs.textT * TEXT_CPS)), (int)box.x + 30, (int)box.y + (cs.speaker ? 60 : 34), 36, WHITE);
	}
	if (TextDone() && (int)(cs.time * 3) % 2 == 0) {
		float x = cs.note ? GetScreenWidth() / 2.0f + 260 : GetScreenWidth() - 100.0f;
		float y = cs.note ? GetScreenHeight() / 2.0f + 110 : GetScreenHeight() - 74.0f;
		DrawTriangle((Vector2){x, y + 12}, (Vector2){x + 10, y}, (Vector2){x - 10, y}, cs.note ? (Color){80, 40, 30, 255} : WHITE);
	}
}

void Cutscene_Draw(void) {
	if (!cs.active) { return; }
	int sw = GetScreenWidth(), sh = GetScreenHeight();
	float amp = cs.shake > 0 ? cs.shakeAmp : 0;
	Vector2 off = {-cs.cam + sinf(cs.time * 70) * amp, cosf(cs.time * 55) * amp * 0.6f};

	ClearBackground(cs.background == ART_JARDIM ? DARKGREEN : (Color){190, 190, 190, 255});
	Texture2D bg = cs.art[cs.background];
	if (bg.id) {
		DrawTexturePro(bg, (Rectangle){0, 0, (float)bg.width, (float)bg.height}, (Rectangle){off.x, off.y, bg.width * (float)sh / bg.height, (float)sh}, (Vector2){0, 0}, 0, WHITE);
	}

	const ActorId order[A_COUNT] = {A_BILHETE, A_IRMA, A_GAMBIT, A_NARA};
	for (int i = 0; i < A_COUNT; i++) {
		const Actor* a = &cs.a[order[i]];
		if (!a->on) { continue; }
		Vector2 feet = {a->pos.x + off.x + (a->wiggle > 0 ? sinf(cs.time * 50) * 4 : 0), a->pos.y + off.y};
		float lift = a->hop > 0 ? fabsf(sinf(cs.time * 9 + order[i])) * 26 : 0;
		if (order[i] != A_BILHETE) { DrawEllipse((int)feet.x, (int)feet.y, a->h * 0.22f * (1 - lift / 80), 8, Fade(BLACK, 0.3f)); }
		DrawActor(order[i], a, (Vector2){feet.x, feet.y - lift});
	}

	if (cs.flash > 0) { DrawRectangle(0, 0, sw, sh, Fade(WHITE, cs.flash / 0.35f)); }
	if (cs.fade > 0) { DrawRectangle(0, 0, sw, sh, Fade(BLACK, cs.fade)); }
	DrawDialogue();
	DrawText("[X] skip", sw - 110, 15, 18, Fade(LIGHTGRAY, 0.6f));
}
