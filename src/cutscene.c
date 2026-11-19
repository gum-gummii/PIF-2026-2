#include <cutscene.h>
#include <math.h>
#include <string.h>

#define TEXT_CPS     38.0f
#define LAUGH_TIME   2.4f
#define JAW_TIME     0.6f
#define EMERGE_TIME  1.4f

typedef enum { A_NARA, A_IRMA, A_DRAGAO, A_COUNT } ActorId;
typedef enum { P_IDLE, P_WALK, P_HIDDEN, P_GIGGLE, P_LAUGH, P_DEAD, P_JAW, P_EMERGE, P_ROAR } Pose;
typedef enum { CS_SHOW, CS_HIDE, CS_WALK, CS_POSE, CS_WIGGLE, CS_CAMERA, CS_SAY, CS_WAIT, CS_SHAKE, CS_FLASH, CS_FADE, CS_END } StepKind;
typedef enum { ART_FUNDO, ART_IRMA, ART_IRMA_RINDO, ART_IRMA_X, ART_DRAGAO, ART_DRAGAO_RUGINDO, ART_COUNT } ArtId;

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
typedef struct { Texture2D tex; int frames; bool ok; } Sprite;

typedef struct {
	bool on;
	Vector2 pos;
	Tween x, y;
	Pose pose;
	float poseT, poseDur, wiggle, h;
} Actor;

static const char* ART_FILES[ART_COUNT] = {
	"karddkassino_bossroom.png", "cutscene/irma.png", "cutscene/irma_rindo.png",
	"cutscene/irma_x.png", "cutscene/dragao.png", "cutscene/dragao_rugindo.png",
};
static const Color ART_COLORS[ART_COUNT] = {
	{0}, {70, 170, 160, 255}, {240, 150, 40, 255}, {200, 180, 170, 255}, {120, 30, 45, 255}, {170, 40, 50, 255},
};

static const Step SCRIPT[] = {
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
	{.kind = CS_FLASH},
	{CS_POSE, A_IRMA, .pose = P_DEAD, .time = 0.7f},
	{CS_SHAKE, .x = 2, .time = JAW_TIME, .together = true},
	{CS_POSE, A_IRMA, .pose = P_JAW, .time = JAW_TIME},
	{CS_WAIT, .time = 0.25f},
	{CS_SHAKE, .x = 6, .time = EMERGE_TIME, .together = true},
	{CS_SHOW, A_DRAGAO, 1150, 460, .pose = P_EMERGE},
	{CS_WAIT, .time = EMERGE_TIME + 0.3f},
	// quadro 7
	{.kind = CS_HIDE, .who = A_IRMA},
	{CS_POSE, A_DRAGAO, .pose = P_ROAR},
	{CS_SHAKE, .x = 10, .time = 1.4f, .together = true},
	{CS_SAY, .text = "* Roars"},
	// quadro 8
	{CS_POSE, A_DRAGAO, .pose = P_IDLE},
	{CS_SAY, .speaker = "Nara", .text = "You will PAY FOR THAT!"},
	{CS_FADE, .x = 1, .time = 0.6f},
	{.kind = CS_END},
};
#define STEP_COUNT ((int)(sizeof(SCRIPT) / sizeof(SCRIPT[0])))

static struct {
	bool active, loaded;
	int step;
	float stepT, wait, time, lock;
	Actor a[A_COUNT];
	float cam, fade, shake, shakeAmp, flash;
	Tween camW, fadeW;
	const char* speaker;
	const char* text;
	float textT;
	Sprite art[ART_COUNT];
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
	case CS_WALK:   a->x = To(a->pos.x, s->x, s->time); a->y = To(a->pos.y, s->y, s->time); a->pose = P_WALK; return s->time;
	case CS_POSE:   a->pose = s->pose; a->poseT = 0; a->poseDur = s->time; return s->time;
	case CS_WIGGLE: a->wiggle = s->time; return s->time;
	case CS_CAMERA: cs.camW = To(cs.cam, s->x, s->time); return s->time;
	case CS_SAY:    cs.speaker = s->speaker; cs.text = s->text; cs.textT = 0; return -1;
	case CS_WAIT:   return s->time;
	case CS_SHAKE:  cs.shake = s->time; cs.shakeAmp = s->x; return s->time;
	case CS_FLASH:  cs.flash = 0.35f; return 0;
	case CS_FADE:   cs.fadeW = To(cs.fade, s->x, s->time); return s->time;
	default:        cs.active = false; return 0;
	}
}

static void RunSteps(void) {
	for (; cs.active && cs.step < STEP_COUNT; cs.step++) {
		const Step* s = &SCRIPT[cs.step];
		cs.stepT = 0;
		cs.wait = StartStep(s);
		if (!s->together && (cs.wait != 0 || s->kind == CS_SAY)) { return; }
	}
	cs.active = false;
}

void Cutscene_StartBoss(Texture2D naraSheet, Rectangle naraFrame) {
	if (!cs.loaded) {
		for (int i = 0; i < ART_COUNT; i++) {
			Sprite* s = &cs.art[i];
			s->ok = FileExists(ART_FILES[i]);
			if (s->ok) { s->tex = LoadTexture(ART_FILES[i]); s->ok = s->tex.id != 0; }
			s->frames = (s->ok && i != ART_FUNDO && s->tex.width >= s->tex.height) ? s->tex.width / s->tex.height : 1;
		}
		cs.loaded = true;
	}
	memset(cs.a, 0, sizeof(cs.a));
	cs.a[A_NARA].h = 150;
	cs.a[A_IRMA].h = 150;
	cs.a[A_DRAGAO].h = 330;
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

void Cutscene_Stop(void) { cs.active = false; }
bool Cutscene_IsActive(void) { return cs.active; }

void Cutscene_Unload(void) {
	for (int i = 0; i < ART_COUNT; i++) { if (cs.art[i].ok) { UnloadTexture(cs.art[i].tex); } }
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
		a->pos = (Vector2){Advance(&a->x, dt, a->pos.x), Advance(&a->y, dt, a->pos.y)};
		if (a->pose == P_WALK && a->x.t >= a->x.dur) { a->pose = P_IDLE; }
	}

	bool ok = cs.lock <= 0 && (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_ENTER));
	if (cs.lock <= 0 && IsKeyPressed(KEY_X)) { cs.active = false; return; }

	if (SCRIPT[cs.step].kind == CS_SAY) {
		if (!ok) { return; }
		if (!TextDone()) { cs.textT = 999; return; }
		cs.text = cs.speaker = NULL;
	} else if ((cs.stepT += dt) < cs.wait) {
		return;
	}
	cs.step++;
	RunSteps();
}

static void DrawPiece(const Sprite* s, ArtId id, Rectangle src, Rectangle dst, Color tint) {
	if (s->ok) { DrawTexturePro(s->tex, src, dst, (Vector2){0, 0}, 0, tint); return; }
	DrawRectangleRec(dst, ColorTint(ART_COLORS[id], tint));
	DrawRectangleLinesEx(dst, 2, BLACK);
}

static void DrawImg(ArtId id, int frame, Vector2 feet, float h, Color tint, float gap) {
	const Sprite* s = &cs.art[id];
	float fw = s->ok ? (float)s->tex.width / s->frames : 1, fh = s->ok ? (float)s->tex.height : 1;
	float w = s->ok ? fw * h / fh : h * (id >= ART_DRAGAO ? 1.0f : 0.5f);
	frame = s->ok ? (frame % s->frames) : 0;
	Rectangle src = {fw * frame, 0, fw, fh}, dst = {feet.x - w / 2, feet.y - h, w, h};

	if (gap > 0) {
		DrawEllipse((int)feet.x, (int)(feet.y - h / 2 - gap / 2), w * 0.35f, gap / 2 + 2, (Color){70, 5, 20, 255});
		DrawPiece(s, id, (Rectangle){src.x, fh / 2, fw, fh / 2}, (Rectangle){dst.x, feet.y - h / 2, w, h / 2}, tint);
		DrawPiece(s, id, (Rectangle){src.x, 0, fw, fh / 2}, (Rectangle){dst.x, feet.y - h - gap, w, h / 2}, tint);
	} else {
		DrawPiece(s, id, src, dst, tint);
	}
	if (!s->ok) {
		const char* tag = TextFormat("[%s]", ART_FILES[id]);
		DrawText(tag, (int)feet.x - MeasureText(tag, 14) / 2, (int)feet.y + 6, 14, Fade(LIGHTGRAY, 0.8f));
	}
}

static float JawGap(const Actor* a) {
	return a->pose == P_JAW ? a->h * 0.6f * 0.55f * Ease(a->poseT / a->poseDur) : 0;
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

	if (id == A_NARA) {
		Rectangle f = cs.frame;
		Rectangle src = {f.width * (a->pose == P_WALK ? anim % 4 : 3), a->pose == P_WALK ? f.height * 4 : 0, f.width, f.height};
		float w = f.width * a->h / f.height;
		DrawTexturePro(cs.nara, src, (Rectangle){feet.x - w / 2, feet.y - a->h, w, a->h}, (Vector2){0, 0}, 0, WHITE);
	} else if (id == A_IRMA) {
		Vector2 head = {feet.x, feet.y - a->h};
		switch (a->pose) {
		case P_HIDDEN: DrawImg(ART_IRMA, 0, feet, a->h, (Color){30, 25, 45, 255}, 0); break;
		case P_GIGGLE: DrawImg(ART_IRMA_RINDO, anim / 2 % 2, feet, a->h, WHITE, 0); DrawLaugh("HE", head, 0.2f); break;
		case P_LAUGH:  DrawImg(ART_IRMA_RINDO, (int)(k * (cs.art[ART_IRMA_RINDO].frames - 1) + 0.5f), feet, a->h, WHITE, 0); DrawLaugh("HA", head, k); break;
		case P_DEAD:
		case P_JAW:    DrawImg(ART_IRMA_X, 0, feet, a->h * 0.6f, WHITE, JawGap(a)); break;
		default:       DrawImg(ART_IRMA, a->pose == P_WALK ? anim : 0, feet, a->h, WHITE, 0); break;
		}
	} else {
		float grow = 1;
		if (a->pose == P_EMERGE) {
			const Actor* irma = &cs.a[A_IRMA];
			float g = Ease(a->poseT / EMERGE_TIME);
			Vector2 mouth = {irma->pos.x + feet.x - a->pos.x, irma->pos.y + feet.y - a->pos.y - irma->h * 0.3f - JawGap(irma) / 2};
			grow = 0.06f + 0.94f * g;
			feet = (Vector2){Lerp(mouth.x, feet.x, g), Lerp(mouth.y, feet.y, g) - sinf(PI * g) * 70};
		}
		bool roar = a->pose == P_ROAR && cs.art[ART_DRAGAO_RUGINDO].ok;
		DrawImg(roar ? ART_DRAGAO_RUGINDO : ART_DRAGAO, anim, feet, a->h * grow * (1 + 0.015f * sinf(cs.time * 3)), WHITE, 0);
	}
}

static void DrawDialogue(void) {
	if (!cs.text) { return; }
	Rectangle box = {60, (float)GetScreenHeight() - 200, (float)GetScreenWidth() - 120, 160};
	DrawRectangleRec(box, Fade(BLACK, 0.92f));
	DrawRectangleLinesEx(box, 5, WHITE);
	DrawRectangleLinesEx((Rectangle){box.x + 5, box.y + 5, box.width - 10, box.height - 10}, 2, (Color){160, 60, 220, 255});
	if (cs.speaker) { DrawText(cs.speaker, (int)box.x + 30, (int)box.y + 18, 26, YELLOW); }
	DrawText(TextSubtext(cs.text, 0, (int)(cs.textT * TEXT_CPS)), (int)box.x + 30, (int)box.y + (cs.speaker ? 60 : 34), 36, WHITE);
	if (TextDone() && (int)(cs.time * 3) % 2 == 0) {
		float x = box.x + box.width - 40, y = box.y + box.height - 34;
		DrawTriangle((Vector2){x, y + 12}, (Vector2){x + 10, y}, (Vector2){x - 10, y}, WHITE);
	}
}

void Cutscene_Draw(void) {
	if (!cs.active) { return; }
	int sw = GetScreenWidth(), sh = GetScreenHeight();
	float amp = cs.shake > 0 ? cs.shakeAmp : 0;
	Vector2 off = {-cs.cam + sinf(cs.time * 70) * amp, cosf(cs.time * 55) * amp * 0.6f};

	ClearBackground((Color){190, 190, 190, 255});
	const Sprite* bg = &cs.art[ART_FUNDO];
	if (bg->ok) {
		float w = bg->tex.width * (float)sh / bg->tex.height;
		DrawTexturePro(bg->tex, (Rectangle){0, 0, (float)bg->tex.width, (float)bg->tex.height}, (Rectangle){off.x, off.y, w, (float)sh}, (Vector2){0, 0}, 0, WHITE);
	}

	const ActorId order[A_COUNT] = {A_IRMA, A_DRAGAO, A_NARA};
	for (int i = 0; i < A_COUNT; i++) {
		const Actor* a = &cs.a[order[i]];
		if (!a->on) { continue; }
		Vector2 feet = {a->pos.x + off.x + (a->wiggle > 0 ? sinf(cs.time * 50) * 4 : 0), a->pos.y + off.y};
		if (a->pose != P_EMERGE) { DrawEllipse((int)feet.x, (int)feet.y, a->h * 0.22f, 8, Fade(BLACK, 0.3f)); }
		DrawActor(order[i], a, feet);
	}

	if (cs.flash > 0) { DrawRectangle(0, 0, sw, sh, Fade(WHITE, cs.flash / 0.35f)); }
	if (cs.fade > 0) { DrawRectangle(0, 0, sw, sh, Fade(BLACK, cs.fade)); }
	DrawDialogue();
	DrawText("[X] pular", sw - 120, 15, 18, Fade(LIGHTGRAY, 0.6f));
}
