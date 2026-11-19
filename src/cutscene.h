#ifndef CUTSCENE_H
#define CUTSCENE_H

#include <raylib.h>

typedef enum {
	ACTOR_NONE = -1,
	ACTOR_NARA,
	ACTOR_IRMA,
	ACTOR_DRAGAO,
	ACTOR_COUNT
} ActorId;

typedef enum {
	POSE_IDLE,
	POSE_WALK,
	POSE_HIDDEN,
	POSE_GIGGLE,
	POSE_LAUGH,
	POSE_DEAD,
	POSE_JAW_OPEN,
	POSE_EMERGE,
	POSE_ROAR
} ActorPose;

typedef enum {
	ART_FUNDO,
	ART_IRMA,
	ART_IRMA_RINDO,
	ART_IRMA_X,
	ART_DRAGAO,
	ART_DRAGAO_RUGINDO,
	ART_COUNT
} CutsceneArt;

typedef enum {
	CS_SHOW,
	CS_HIDE,
	CS_WALK,
	CS_POSE,
	CS_WIGGLE,
	CS_CAMERA,
	CS_SAY,
	CS_WAIT,
	CS_SHAKE,
	CS_FLASH,
	CS_FADE,
	CS_END
} CutsceneStepKind;

typedef struct {
	CutsceneStepKind kind;
	ActorId actor;
	float x, y;
	float time;
	ActorPose pose;
	const char* speaker;
	const char* text;
	bool together;
} CutsceneStep;

typedef struct {
	Texture2D tex;
	int frames;
	bool ok;
} CutsceneSprite;

typedef struct {
	bool visible;
	Vector2 pos;
	Vector2 from;
	Vector2 to;
	float moveT;
	float moveDur;
	ActorPose pose;
	float poseT;
	float poseDur;
	float wiggleT;
	float height;
} CutsceneActor;

typedef struct {
	bool active;
	const CutsceneStep* steps;
	int stepCount;
	int step;
	float stepT;
	float stepWait;
	float time;
	float inputLock;

	CutsceneActor actors[ACTOR_COUNT];

	float camX, camFrom, camTo, camT, camDur;
	float fade, fadeFrom, fadeTo, fadeT, fadeDur;
	float shakeT, shakeAmp;
	float flashT;

	const char* speaker;
	const char* text;
	float textT;

	CutsceneSprite art[ART_COUNT];
	bool artLoaded;
	Texture2D nara;
	Rectangle naraFrame;
	float worldW;
} Cutscene;

void Cutscene_StartBoss(Cutscene* cs, Texture2D naraSheet, Rectangle naraFrame);
void Cutscene_Update(Cutscene* cs, float dt);
void Cutscene_Draw(const Cutscene* cs);
void Cutscene_Stop(Cutscene* cs);
bool Cutscene_IsActive(const Cutscene* cs);
void Cutscene_Unload(Cutscene* cs);

#endif
