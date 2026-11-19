#include <cutscene.h>
#include <math.h>
#include <string.h>

#define TEXT_CPS     38.0f
#define LAUGH_TIME   2.4f
#define EMERGE_TIME  1.4f
#define JAW_TIME     0.6f
#define NARA_H       150.0f
#define IRMA_H       150.0f
#define DRAGAO_H     330.0f

static const char* ART_FILES[ART_COUNT] = {
	[ART_FUNDO]          = "karddkassino_bossroom.png",
	[ART_IRMA]           = "cutscene/irma.png",
	[ART_IRMA_RINDO]     = "cutscene/irma_rindo.png",
	[ART_IRMA_X]         = "cutscene/irma_x.png",
	[ART_DRAGAO]         = "cutscene/dragao.png",
	[ART_DRAGAO_RUGINDO] = "cutscene/dragao_rugindo.png",
};

static const CutsceneStep BOSS_SCRIPT[] = {
	// quadro 1
	{.kind = CS_SHOW, .actor = ACTOR_NARA, .x = -80, .y = 440, .pose = POSE_WALK},
	{.kind = CS_SHOW, .actor = ACTOR_IRMA, .x = 1590, .y = 255, .pose = POSE_HIDDEN},
	{.kind = CS_FADE, .x = 0, .time = 0.8f, .together = true},
	{.kind = CS_WALK, .actor = ACTOR_NARA, .x = 430, .y = 440, .time = 2.2f},
	{.kind = CS_SAY, .speaker = "Nara", .text = "Show yourself, tyrant!"},

	// quadro 2
	{.kind = CS_CAMERA, .x = 488, .time = 1.4f},
	{.kind = CS_WAIT, .time = 0.5f},
	{.kind = CS_WIGGLE, .actor = ACTOR_IRMA, .time = 0.9f},
	{.kind = CS_WAIT, .time = 0.3f},

	// quadro 3
	{.kind = CS_CAMERA, .x = 140, .time = 1.8f, .together = true},
	{.kind = CS_WALK, .actor = ACTOR_IRMA, .x = 1120, .y = 440, .time = 1.8f},
	{.kind = CS_POSE, .actor = ACTOR_IRMA, .pose = POSE_GIGGLE, .time = 1.2f},

	// quadro 4
	{.kind = CS_SAY, .speaker = "Nara", .text = "WHAT'S SO FUNNY, HUH?"},

	// quadro 5
	{.kind = CS_POSE, .actor = ACTOR_IRMA, .pose = POSE_IDLE, .time = 0.5f},
	{.kind = CS_SAY, .speaker = "Nara", .text = "S-sis..?"},
	{.kind = CS_SAY, .speaker = "Nara", .text = "What happened to you..?"},

	// quadro 6
	{.kind = CS_SHAKE, .x = 3, .time = LAUGH_TIME, .together = true},
	{.kind = CS_POSE, .actor = ACTOR_IRMA, .pose = POSE_LAUGH, .time = LAUGH_TIME},
	{.kind = CS_FLASH},
	{.kind = CS_POSE, .actor = ACTOR_IRMA, .pose = POSE_DEAD, .time = 0.7f},
	{.kind = CS_SHAKE, .x = 2, .time = JAW_TIME, .together = true},
	{.kind = CS_POSE, .actor = ACTOR_IRMA, .pose = POSE_JAW_OPEN, .time = JAW_TIME},
	{.kind = CS_WAIT, .time = 0.25f},
	{.kind = CS_SHAKE, .x = 6, .time = EMERGE_TIME, .together = true},
	{.kind = CS_SHOW, .actor = ACTOR_DRAGAO, .x = 1150, .y = 460, .pose = POSE_EMERGE},
	{.kind = CS_WAIT, .time = EMERGE_TIME + 0.3f},

	// quadro 7
	{.kind = CS_HIDE, .actor = ACTOR_IRMA},
	{.kind = CS_POSE, .actor = ACTOR_DRAGAO, .pose = POSE_ROAR},
	{.kind = CS_SHAKE, .x = 10, .time = 1.4f, .together = true},
	{.kind = CS_SAY, .text = "* Roars"},

	// quadro 8
	{.kind = CS_POSE, .actor = ACTOR_DRAGAO, .pose = POSE_IDLE},
	{.kind = CS_SAY, .speaker = "Nara", .text = "You will PAY FOR THAT!"},
	{.kind = CS_FADE, .x = 1, .time = 0.6f},
	{.kind = CS_END},
};

static float Clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
static float Ease(float t) { t = Clamp01(t); return t * t * (3 - 2 * t); }
static float Lerp(float a, float c, float t) { return a + (c - a) * t; }

static bool Confirm(void) { return IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_ENTER); }

static void LoadArt(Cutscene* cs) {
	if (cs->artLoaded) { return; }
	for (int i = 0; i < ART_COUNT; i++) {
		CutsceneSprite* s = &cs->art[i];
		s->ok = FileExists(ART_FILES[i]);
		if (!s->ok) { continue; }
		s->tex = LoadTexture(ART_FILES[i]);
		s->ok = s->tex.id != 0;
		s->frames = (i == ART_FUNDO || s->tex.height <= 0) ? 1 : s->tex.width / s->tex.height;
		if (s->frames < 1) { s->frames = 1; }
	}
	cs->artLoaded = true;
}

void Cutscene_Unload(Cutscene* cs) {
	if (!cs->artLoaded) { return; }
	for (int i = 0; i < ART_COUNT; i++) {
		if (cs->art[i].ok) { UnloadTexture(cs->art[i].tex); }
		cs->art[i].ok = false;
	}
	cs->artLoaded = false;
}

static void SetPose(CutsceneActor* a, ActorPose pose, float dur) {
	a->pose = pose;
	a->poseT = 0;
	a->poseDur = dur;
}

static float StartStep(Cutscene* cs, const CutsceneStep* s) {
	CutsceneActor* a = (s->actor >= 0 && s->actor < ACTOR_COUNT) ? &cs->actors[s->actor] : NULL;

	switch (s->kind) {
	case CS_SHOW:
		if (!a) { return 0; }
		a->visible = true;
		a->pos = a->from = a->to = (Vector2){s->x, s->y};
		a->moveDur = 0;
		SetPose(a, s->pose, 0);
		return 0;
	case CS_HIDE:
		if (a) { a->visible = false; }
		return 0;
	case CS_WALK:
		if (!a) { return 0; }
		a->from = a->pos;
		a->to = (Vector2){s->x, s->y};
		a->moveT = 0;
		a->moveDur = s->time;
		SetPose(a, POSE_WALK, 0);
		return s->time;
	case CS_POSE:
		if (a) { SetPose(a, s->pose, s->time); }
		return s->time;
	case CS_WIGGLE:
		if (a) { a->wiggleT = s->time; }
		return s->time;
	case CS_CAMERA:
		cs->camFrom = cs->camX;
		cs->camTo = s->x;
		cs->camT = 0;
		cs->camDur = s->time;
		return s->time;
	case CS_SAY:
		cs->speaker = s->speaker;
		cs->text = s->text;
		cs->textT = 0;
		return -1;
	case CS_WAIT:
		return s->time;
	case CS_SHAKE:
		cs->shakeT = s->time;
		cs->shakeAmp = s->x;
		return s->time;
	case CS_FLASH:
		cs->flashT = 0.35f;
		return 0;
	case CS_FADE:
		cs->fadeFrom = cs->fade;
		cs->fadeTo = s->x;
		cs->fadeT = 0;
		cs->fadeDur = s->time;
		return s->time;
	case CS_END:
	default:
		cs->active = false;
		return 0;
	}
}

static void RunSteps(Cutscene* cs) {
	while (cs->active && cs->step < cs->stepCount) {
		const CutsceneStep* s = &cs->steps[cs->step];
		float wait = StartStep(cs, s);
		cs->stepT = 0;
		if (s->together || (wait == 0 && s->kind != CS_SAY)) {
			cs->step++;
			continue;
		}
		cs->stepWait = wait;
		return;
	}
	cs->active = false;
}

void Cutscene_StartBoss(Cutscene* cs, Texture2D naraSheet, Rectangle naraFrame) {
	CutsceneSprite art[ART_COUNT];
	bool loaded = cs->artLoaded;
	memcpy(art, cs->art, sizeof(art));
	memset(cs, 0, sizeof(*cs));
	memcpy(cs->art, art, sizeof(art));
	cs->artLoaded = loaded;
	LoadArt(cs);

	cs->active = true;
	cs->steps = BOSS_SCRIPT;
	cs->stepCount = (int)(sizeof(BOSS_SCRIPT) / sizeof(BOSS_SCRIPT[0]));
	cs->nara = naraSheet;
	cs->naraFrame = naraFrame;
	cs->fade = 1.0f;
	cs->inputLock = 0.3f;

	const CutsceneSprite* bg = &cs->art[ART_FUNDO];
	cs->worldW = bg->ok ? bg->tex.width * ((float)GetScreenHeight() / bg->tex.height) : 1768.0f;

	cs->actors[ACTOR_NARA].height = NARA_H;
	cs->actors[ACTOR_IRMA].height = IRMA_H;
	cs->actors[ACTOR_DRAGAO].height = DRAGAO_H;

	RunSteps(cs);
}

void Cutscene_Stop(Cutscene* cs) { cs->active = false; }
bool Cutscene_IsActive(const Cutscene* cs) { return cs->active; }

static bool TextDone(const Cutscene* cs) {
	return cs->text == NULL || (int)(cs->textT * TEXT_CPS) >= (int)strlen(cs->text);
}

void Cutscene_Update(Cutscene* cs, float dt) {
	if (!cs->active) { return; }

	cs->time += dt;
	if (cs->inputLock > 0) { cs->inputLock -= dt; }

	for (int i = 0; i < ACTOR_COUNT; i++) {
		CutsceneActor* a = &cs->actors[i];
		a->poseT += dt;
		if (a->wiggleT > 0) { a->wiggleT -= dt; }
		if (a->moveDur > 0 && a->moveT < a->moveDur) {
			a->moveT += dt;
			float k = Ease(a->moveT / a->moveDur);
			a->pos = (Vector2){Lerp(a->from.x, a->to.x, k), Lerp(a->from.y, a->to.y, k)};
			if (a->moveT >= a->moveDur && a->pose == POSE_WALK) { SetPose(a, POSE_IDLE, 0); }
		}
	}

	if (cs->camDur > 0 && cs->camT < cs->camDur) {
		cs->camT += dt;
		cs->camX = Lerp(cs->camFrom, cs->camTo, Ease(cs->camT / cs->camDur));
	}
	if (cs->fadeDur > 0 && cs->fadeT < cs->fadeDur) {
		cs->fadeT += dt;
		cs->fade = Lerp(cs->fadeFrom, cs->fadeTo, Clamp01(cs->fadeT / cs->fadeDur));
	}
	if (cs->shakeT > 0) { cs->shakeT -= dt; }
	if (cs->flashT > 0) { cs->flashT -= dt; }
	cs->textT += dt;

	if (cs->inputLock <= 0 && IsKeyPressed(KEY_X)) {
		cs->active = false;
		return;
	}

	const CutsceneStep* s = &cs->steps[cs->step];
	if (s->kind == CS_SAY) {
		if (cs->inputLock > 0 || !Confirm()) { return; }
		if (!TextDone(cs)) {
			cs->textT = 999.0f;
			return;
		}
		cs->text = NULL;
		cs->speaker = NULL;
	} else {
		cs->stepT += dt;
		if (cs->stepT < cs->stepWait) { return; }
	}
	cs->step++;
	RunSteps(cs);
}

static void DrawArt(const CutsceneSprite* s, int frame, Vector2 feet, float h, Color tint) {
	float fw = (float)s->tex.width / s->frames, fh = (float)s->tex.height;
	float scale = h / fh;
	frame = frame < 0 ? 0 : (frame >= s->frames ? s->frames - 1 : frame);
	Rectangle src = {fw * frame, 0, fw, fh};
	Rectangle dst = {feet.x - fw * scale / 2, feet.y - h, fw * scale, h};
	DrawTexturePro(s->tex, src, dst, (Vector2){0, 0}, 0, tint);
}

static void DrawMissingTag(CutsceneArt art, Vector2 feet) {
	const char* tag = TextFormat("[%s]", ART_FILES[art]);
	DrawText(tag, (int)feet.x - MeasureText(tag, 14) / 2, (int)feet.y + 6, 14, Fade(LIGHTGRAY, 0.7f));
}

static void Tri(Vector2 a, Vector2 c, Vector2 d, Color col) {
	float cross = (c.x - a.x) * (d.y - a.y) - (c.y - a.y) * (d.x - a.x);
	if (cross > 0) { DrawTriangle(a, d, c, col); }
	else { DrawTriangle(a, c, d, col); }
}

static void DrawIrmaDoll(Vector2 feet, float h, float laugh, bool dark) {
	Color skin = dark ? (Color){25, 20, 35, 255} : (Color){235, 200, 170, 255};
	Color dress = dark ? (Color){20, 15, 30, 255} : (Color){70, 170, 160, 255};
	Color hair = dark ? (Color){15, 10, 25, 255} : (Color){240, 150, 40, 255};
	Color legs = dark ? (Color){15, 10, 25, 255} : (Color){60, 40, 40, 255};

	float headR = h * 0.17f * (1.0f + laugh * 0.9f);
	Vector2 head = {feet.x, feet.y - h * 0.78f - laugh * h * 0.1f};

	DrawRectangle((int)(feet.x - h * 0.1f), (int)(feet.y - h * 0.2f), (int)(h * 0.06f), (int)(h * 0.2f), legs);
	DrawRectangle((int)(feet.x + h * 0.04f), (int)(feet.y - h * 0.2f), (int)(h * 0.06f), (int)(h * 0.2f), legs);
	Tri((Vector2){feet.x, feet.y - h * 0.64f}, (Vector2){feet.x - h * 0.24f, feet.y - h * 0.16f}, (Vector2){feet.x + h * 0.24f, feet.y - h * 0.16f}, dress);

	if (laugh < 0.6f) {
		for (int i = 0; i < 7; i++) {
			float ang = -PI + PI * (i + 0.5f) / 7.0f;
			Vector2 tip = {head.x + cosf(ang) * headR * 1.75f, head.y + sinf(ang) * headR * 1.75f};
			Vector2 b1 = {head.x + cosf(ang - 0.35f) * headR * 0.9f, head.y + sinf(ang - 0.35f) * headR * 0.9f};
			Vector2 b2 = {head.x + cosf(ang + 0.35f) * headR * 0.9f, head.y + sinf(ang + 0.35f) * headR * 0.9f};
			Tri(tip, b1, b2, hair);
		}
	}
	DrawCircleV(head, headR, skin);
	if (dark) {
		DrawCircleV((Vector2){head.x - headR * 0.35f, head.y - headR * 0.1f}, headR * 0.12f, (Color){230, 40, 60, 255});
		DrawCircleV((Vector2){head.x + headR * 0.35f, head.y - headR * 0.1f}, headR * 0.12f, (Color){230, 40, 60, 255});
		return;
	}

	float eyeR = headR * (laugh < 0.6f ? 0.1f : 0.2f);
	for (int sgn = -1; sgn <= 1; sgn += 2) {
		Vector2 e = {head.x + sgn * headR * 0.38f, head.y - headR * 0.2f};
		if (laugh >= 0.6f) {
			DrawCircleV(e, eyeR, WHITE);
			DrawCircleV(e, eyeR * 0.35f, BLACK);
		} else {
			DrawCircleV(e, eyeR, BLACK);
		}
	}

	float open = 0.15f + laugh * 0.85f;
	Vector2 m = {head.x, head.y + headR * 0.35f};
	float mw = headR * 0.6f * (0.5f + open), mh = headR * 0.45f * open + 2;
	DrawEllipse((int)m.x, (int)m.y, mw, mh, (Color){120, 20, 40, 255});
	if (laugh > 0.5f) {
		int teeth = 7;
		for (int i = 0; i < teeth; i++) {
			float tx = m.x - mw * 0.85f + (mw * 1.7f) * (i + 0.5f) / teeth;
			float tw = mw * 1.7f / teeth * 0.5f;
			Tri((Vector2){tx - tw, m.y - mh * 0.7f}, (Vector2){tx + tw, m.y - mh * 0.7f}, (Vector2){tx, m.y - mh * 0.15f}, WHITE);
			Tri((Vector2){tx - tw, m.y + mh * 0.7f}, (Vector2){tx + tw, m.y + mh * 0.7f}, (Vector2){tx, m.y + mh * 0.15f}, WHITE);
		}
	}
}

static float JawOpen(const CutsceneActor* a) {
	if (a->pose != POSE_JAW_OPEN) { return 0; }
	return a->poseDur > 0 ? Ease(a->poseT / a->poseDur) : 1;
}

static float DeadHeight(const Cutscene* cs, const CutsceneActor* a) {
	(void)cs;
	return a->height * 0.6f;
}

static float DeadGap(const Cutscene* cs, const CutsceneActor* a, float open) {
	float hd = DeadHeight(cs, a);
	if (cs->art[ART_IRMA_X].ok) { return hd * 0.55f * open; }
	float r = hd * 0.5f;
	return r * (0.25f + 1.1f * open);
}

static Vector2 DeadMouth(const Cutscene* cs, const CutsceneActor* a, Vector2 feet) {
	float split = feet.y - DeadHeight(cs, a) * 0.5f;
	return (Vector2){feet.x, split - DeadGap(cs, a, JawOpen(a)) * 0.5f};
}

static void DrawIrmaDeadDoll(Vector2 feet, float hd, float gap) {
	Color skin = {200, 180, 170, 255};
	float r = hd * 0.5f;
	Vector2 c = {feet.x, feet.y - r};
	Vector2 top = {c.x, c.y - gap};

	DrawEllipse((int)c.x, (int)(c.y - gap * 0.5f), r * 0.85f, gap * 0.5f + r * 0.12f, (Color){70, 5, 20, 255});
	if (gap > r * 0.5f) { DrawEllipse((int)c.x, (int)(c.y - gap * 0.5f), r * 0.5f, gap * 0.3f, (Color){150, 30, 40, 255}); }
	DrawCircleSector(c, r, 0, 180, 24, skin);
	DrawCircleSector(top, r, 180, 360, 24, skin);

	for (int i = 0; i < 6; i++) {
		float tx = c.x - r * 0.7f + r * 1.4f * (i + 0.5f) / 6.0f;
		float tw = r * 0.09f;
		Tri((Vector2){tx - tw, top.y}, (Vector2){tx + tw, top.y}, (Vector2){tx, top.y + r * 0.24f}, WHITE);
		Tri((Vector2){tx - tw, c.y}, (Vector2){tx + tw, c.y}, (Vector2){tx, c.y - r * 0.2f}, WHITE);
	}

	for (int sgn = -1; sgn <= 1; sgn += 2) {
		Vector2 e = {top.x + sgn * r * 0.4f, top.y - r * 0.45f};
		float k = r * 0.14f;
		DrawLineEx((Vector2){e.x - k, e.y - k}, (Vector2){e.x + k, e.y + k}, 4, BLACK);
		DrawLineEx((Vector2){e.x - k, e.y + k}, (Vector2){e.x + k, e.y - k}, 4, BLACK);
	}
}

static void DrawArtSplit(const CutsceneSprite* s, Vector2 feet, float h, float gap) {
	float fw = (float)s->tex.width / s->frames, fh = (float)s->tex.height;
	float scale = h / fh;
	float w = fw * scale;
	DrawEllipse((int)feet.x, (int)(feet.y - h * 0.5f - gap * 0.5f), w * 0.35f, gap * 0.5f + 2, (Color){70, 5, 20, 255});
	DrawTexturePro(s->tex, (Rectangle){0, fh * 0.5f, fw, fh * 0.5f}, (Rectangle){feet.x - w / 2, feet.y - h * 0.5f, w, h * 0.5f}, (Vector2){0, 0}, 0, WHITE);
	DrawTexturePro(s->tex, (Rectangle){0, 0, fw, fh * 0.5f}, (Rectangle){feet.x - w / 2, feet.y - h - gap, w, h * 0.5f}, (Vector2){0, 0}, 0, WHITE);
}

static void DrawDragonDoll(Vector2 feet, float h, float grow, bool roar, float time) {
	float s = h * grow;
	if (s <= 1) { return; }
	Color body = {120, 30, 45, 255};
	Color dark = {80, 15, 30, 255};
	Color belly = {200, 120, 90, 255};
	Color claw = {245, 235, 210, 255};

	Vector2 t0 = {feet.x + s * 0.3f, feet.y - s * 0.3f};
	Vector2 t1 = {feet.x + s * 0.6f, feet.y - s * 0.2f};
	Vector2 t2 = {feet.x + s * 0.75f, feet.y - s * 0.45f + sinf(time * 3) * s * 0.03f};
	Vector2 t3 = {feet.x + s * 0.68f, feet.y - s * 0.62f};
	DrawLineEx(t0, t1, s * 0.12f, body);
	DrawLineEx(t1, t2, s * 0.08f, body);
	DrawLineEx(t2, t3, s * 0.05f, body);
	Tri(t3, (Vector2){t3.x - s * 0.06f, t3.y - s * 0.08f}, (Vector2){t3.x + s * 0.07f, t3.y - s * 0.06f}, dark);

	float flap = sinf(time * 4) * s * 0.04f;
	Tri((Vector2){feet.x - s * 0.05f, feet.y - s * 0.55f}, (Vector2){feet.x + s * 0.5f, feet.y - s * 1.05f - flap}, (Vector2){feet.x + s * 0.4f, feet.y - s * 0.5f}, dark);
	Tri((Vector2){feet.x + s * 0.05f, feet.y - s * 0.5f}, (Vector2){feet.x + s * 0.3f, feet.y - s * 1.0f + flap}, (Vector2){feet.x + s * 0.25f, feet.y - s * 0.45f}, body);

	DrawEllipse((int)(feet.x + s * 0.1f), (int)(feet.y - s * 0.35f), s * 0.38f, s * 0.26f, body);
	DrawEllipse((int)(feet.x - s * 0.02f), (int)(feet.y - s * 0.3f), s * 0.2f, s * 0.15f, belly);

	for (int i = 0; i < 2; i++) {
		float lx = feet.x + (i == 0 ? -s * 0.12f : s * 0.25f);
		DrawRectangle((int)(lx - s * 0.06f), (int)(feet.y - s * 0.18f), (int)(s * 0.12f), (int)(s * 0.18f), body);
		for (int k = 0; k < 3; k++) {
			float cx = lx - s * 0.06f - s * 0.02f + k * s * 0.05f;
			Tri((Vector2){cx, feet.y}, (Vector2){cx + s * 0.04f, feet.y}, (Vector2){cx - s * 0.02f, feet.y + s * 0.03f}, claw);
		}
	}

	Vector2 neck0 = {feet.x - s * 0.12f, feet.y - s * 0.48f};
	Vector2 head = {feet.x - s * 0.36f, feet.y - s * 0.8f};
	if (roar) { head.y -= s * 0.04f; head.x += sinf(time * 40) * s * 0.01f; }
	DrawLineEx(neck0, head, s * 0.13f, body);
	DrawCircleV(head, s * 0.15f, body);

	Tri((Vector2){head.x + s * 0.02f, head.y - s * 0.1f}, (Vector2){head.x + s * 0.2f, head.y - s * 0.3f}, (Vector2){head.x + s * 0.12f, head.y - s * 0.05f}, claw);
	Tri((Vector2){head.x - s * 0.05f, head.y - s * 0.12f}, (Vector2){head.x + s * 0.05f, head.y - s * 0.33f}, (Vector2){head.x + s * 0.04f, head.y - s * 0.1f}, claw);

	Vector2 snout = {head.x - s * 0.17f, head.y + s * 0.03f};
	float jaw = roar ? s * 0.1f : s * 0.02f;
	DrawEllipse((int)snout.x, (int)(snout.y - jaw * 0.3f), s * 0.13f, s * 0.07f, body);
	DrawEllipse((int)snout.x, (int)(snout.y + jaw), s * 0.12f, s * 0.05f, dark);
	if (roar) {
		DrawEllipse((int)snout.x, (int)(snout.y + jaw * 0.4f), s * 0.1f, jaw * 0.6f, (Color){60, 0, 10, 255});
		for (int i = 0; i < 4; i++) {
			float tx = snout.x - s * 0.09f + i * s * 0.05f;
			Tri((Vector2){tx, snout.y + s * 0.02f}, (Vector2){tx + s * 0.03f, snout.y + s * 0.02f}, (Vector2){tx + s * 0.015f, snout.y + s * 0.06f}, claw);
		}
		for (int i = 0; i < 5; i++) {
			float ang = PI * 0.75f + (i - 2) * 0.22f;
			float r0 = s * 0.25f, r1 = s * (0.4f + 0.06f * sinf(time * 30 + i));
			DrawLineEx((Vector2){snout.x + cosf(ang) * r0, snout.y + sinf(ang) * r0}, (Vector2){snout.x + cosf(ang) * r1, snout.y + sinf(ang) * r1}, 4, Fade(WHITE, 0.8f));
		}
	}
	DrawCircleV((Vector2){head.x - s * 0.04f, head.y - s * 0.03f}, s * 0.03f, YELLOW);
	DrawCircleV((Vector2){head.x - s * 0.045f, head.y - s * 0.03f}, s * 0.012f, BLACK);
}

static void DrawLaughText(const char* word, Vector2 head, float strength, float time) {
	for (int i = 0; i < 3; i++) {
		float phase = fmodf(time * 1.2f + i / 3.0f, 1.0f);
		float side = (i % 2 == 0) ? -1.0f : 1.0f;
		Vector2 p = {head.x + side * (40 + phase * 30), head.y - 30 - phase * 60};
		int fs = (int)(20 + strength * 16);
		DrawText(word, (int)p.x - MeasureText(word, fs) / 2, (int)p.y, fs, Fade(WHITE, 1.0f - phase));
	}
}

static void DrawNara(const Cutscene* cs, const CutsceneActor* a, Vector2 feet) {
	Rectangle f = cs->naraFrame;
	int col = 3, row = 0;
	if (a->pose == POSE_WALK) {
		row = 4;
		col = (int)(cs->time * 10.0f) % 4;
	}
	float scale = a->height / f.height;
	Rectangle src = {f.width * col, f.height * row, f.width, f.height};
	Rectangle dst = {feet.x - f.width * scale / 2, feet.y - a->height, f.width * scale, a->height};
	DrawTexturePro(cs->nara, src, dst, (Vector2){0, 0}, 0, WHITE);
}

static void DrawIrma(const Cutscene* cs, const CutsceneActor* a, Vector2 feet) {
	const CutsceneSprite* idle = &cs->art[ART_IRMA];
	const CutsceneSprite* laughArt = &cs->art[ART_IRMA_RINDO];
	const CutsceneSprite* deadArt = &cs->art[ART_IRMA_X];
	Vector2 head = {feet.x, feet.y - a->height * 0.85f};

	switch (a->pose) {
	case POSE_HIDDEN:
		if (idle->ok) { DrawArt(idle, 0, feet, a->height, (Color){30, 25, 45, 255}); }
		else { DrawIrmaDoll(feet, a->height, 0, true); }
		break;
	case POSE_GIGGLE: {
		float bounce = fabsf(sinf(cs->time * 14.0f)) * 4;
		Vector2 fb = {feet.x, feet.y - bounce};
		if (laughArt->ok) { DrawArt(laughArt, (int)(cs->time * 6.0f) % (laughArt->frames > 1 ? 2 : 1), fb, a->height, WHITE); }
		else { DrawIrmaDoll(fb, a->height, 0.15f + 0.1f * sinf(cs->time * 20.0f), false); DrawMissingTag(ART_IRMA_RINDO, feet); }
		DrawLaughText("HE", head, 0.2f, cs->time);
		break;
	}
	case POSE_LAUGH: {
		float k = a->poseDur > 0 ? Clamp01(a->poseT / a->poseDur) : 1;
		if (laughArt->ok) { DrawArt(laughArt, (int)(k * (laughArt->frames - 1) + 0.5f), feet, a->height, WHITE); }
		else { DrawIrmaDoll(feet, a->height, k, false); DrawMissingTag(ART_IRMA_RINDO, feet); }
		DrawLaughText("HA", (Vector2){head.x, head.y - k * a->height * 0.15f}, k, cs->time * (1 + k));
		break;
	}
	case POSE_DEAD:
	case POSE_JAW_OPEN: {
		float gap = DeadGap(cs, a, JawOpen(a));
		if (deadArt->ok) { DrawArtSplit(deadArt, feet, DeadHeight(cs, a), gap); }
		else { DrawIrmaDeadDoll(feet, DeadHeight(cs, a), gap); DrawMissingTag(ART_IRMA_X, feet); }
		break;
	}
	default:
		if (idle->ok) {
			int frame = (a->pose == POSE_WALK && idle->frames > 1) ? (int)(cs->time * 8.0f) % idle->frames : 0;
			DrawArt(idle, frame, feet, a->height, WHITE);
		} else {
			DrawIrmaDoll(feet, a->height, 0, false);
			DrawMissingTag(ART_IRMA, feet);
		}
		break;
	}
}

static void DrawDragon(const Cutscene* cs, const CutsceneActor* a, Vector2 feet) {
	const CutsceneSprite* idle = &cs->art[ART_DRAGAO];
	const CutsceneSprite* roarArt = &cs->art[ART_DRAGAO_RUGINDO];
	bool roar = a->pose == POSE_ROAR;
	float grow = 1.0f;
	float breathe = 1.0f + 0.015f * sinf(cs->time * 3.0f);

	if (a->pose == POSE_EMERGE) {
		float g = Ease(a->poseT / EMERGE_TIME);
		grow = 0.06f + 0.94f * g;
		const CutsceneActor* irma = &cs->actors[ACTOR_IRMA];
		if (irma->visible) {
			Vector2 off = {feet.x - a->pos.x, feet.y - a->pos.y};
			Vector2 irmaFeet = {irma->pos.x + off.x, irma->pos.y + off.y};
			Vector2 mouth = DeadMouth(cs, irma, irmaFeet);
			mouth.y += grow * a->height * 0.25f * (1.0f - g);
			feet = (Vector2){Lerp(mouth.x, feet.x, g), Lerp(mouth.y, feet.y, g) - sinf(PI * g) * 70.0f};
		}
	}
	float h = a->height * grow * breathe;

	if (roar && roarArt->ok) {
		DrawArt(roarArt, (int)(cs->time * 8.0f) % roarArt->frames, feet, h, WHITE);
	} else if (idle->ok) {
		DrawArt(idle, (int)(cs->time * 6.0f) % idle->frames, feet, h, WHITE);
	} else {
		DrawDragonDoll(feet, a->height * breathe, grow, roar, cs->time);
		if (a->pose != POSE_EMERGE) { DrawMissingTag(ART_DRAGAO, feet); }
	}
}

static void DrawBackground(const Cutscene* cs, float ox, float oy) {
	int sh = GetScreenHeight();
	const CutsceneSprite* bg = &cs->art[ART_FUNDO];
	if (bg->ok) {
		float scale = (float)sh / bg->tex.height;
		DrawTexturePro(bg->tex, (Rectangle){0, 0, (float)bg->tex.width, (float)bg->tex.height},
			(Rectangle){ox, oy, bg->tex.width * scale, (float)sh}, (Vector2){0, 0}, 0, WHITE);
		return;
	}
	DrawRectangle((int)ox, (int)oy, (int)cs->worldW, sh, (Color){190, 190, 190, 255});
	DrawRectangle((int)ox, (int)(oy + 337), (int)(cs->worldW * 0.78f), 106, RED);
	DrawRectangle((int)ox, (int)oy, (int)cs->worldW, 70, (Color){255, 130, 130, 255});
	DrawRectangle((int)ox, (int)(oy + sh - 40), (int)cs->worldW, 40, (Color){255, 130, 130, 255});
}

static void DrawDialogue(const Cutscene* cs) {
	if (cs->text == NULL) { return; }
	int sw = GetScreenWidth(), sh = GetScreenHeight();
	Rectangle box = {60, (float)sh - 200, (float)sw - 120, 160};
	DrawRectangleRec(box, Fade(BLACK, 0.92f));
	DrawRectangleLinesEx(box, 5, WHITE);
	DrawRectangleLinesEx((Rectangle){box.x + 5, box.y + 5, box.width - 10, box.height - 10}, 2, (Color){160, 60, 220, 255});

	int ty = (int)box.y + 34;
	if (cs->speaker) {
		DrawText(cs->speaker, (int)box.x + 30, (int)box.y + 18, 26, YELLOW);
		ty = (int)box.y + 60;
	}
	int n = (int)(cs->textT * TEXT_CPS);
	int len = (int)strlen(cs->text);
	if (n > len) { n = len; }
	DrawText(TextSubtext(cs->text, 0, n), (int)box.x + 30, ty, 36, WHITE);

	if (TextDone(cs) && (int)(cs->time * 3.0f) % 2 == 0) {
		float x = box.x + box.width - 40, y = box.y + box.height - 34;
		Tri((Vector2){x - 10, y}, (Vector2){x + 10, y}, (Vector2){x, y + 12}, WHITE);
	}
}

void Cutscene_Draw(const Cutscene* cs) {
	if (!cs->active) { return; }
	int sw = GetScreenWidth(), sh = GetScreenHeight();

	ClearBackground(BLACK);
	float shx = 0, shy = 0;
	if (cs->shakeT > 0) {
		shx = sinf(cs->time * 70.0f) * cs->shakeAmp;
		shy = cosf(cs->time * 55.0f) * cs->shakeAmp * 0.6f;
	}
	float ox = -cs->camX + shx, oy = shy;
	DrawBackground(cs, ox, oy);

	const ActorId order[ACTOR_COUNT] = {ACTOR_IRMA, ACTOR_DRAGAO, ACTOR_NARA};
	for (int i = 0; i < ACTOR_COUNT; i++) {
		const CutsceneActor* a = &cs->actors[order[i]];
		if (!a->visible) { continue; }
		Vector2 feet = {a->pos.x + ox, a->pos.y + oy};
		if (a->wiggleT > 0) { feet.x += sinf(cs->time * 50.0f) * 4; }
		if (a->pose != POSE_EMERGE) { DrawEllipse((int)feet.x, (int)feet.y, a->height * 0.22f, 8, Fade(BLACK, 0.3f)); }
		switch (order[i]) {
		case ACTOR_NARA:   DrawNara(cs, a, feet); break;
		case ACTOR_IRMA:   DrawIrma(cs, a, feet); break;
		case ACTOR_DRAGAO: DrawDragon(cs, a, feet); break;
		default: break;
		}
	}

	if (cs->flashT > 0) { DrawRectangle(0, 0, sw, sh, Fade(WHITE, cs->flashT / 0.35f)); }
	if (cs->fade > 0) { DrawRectangle(0, 0, sw, sh, Fade(BLACK, cs->fade)); }
	DrawDialogue(cs);
	DrawText("[X] pular", sw - 120, 15, 18, Fade(LIGHTGRAY, 0.6f));
}
