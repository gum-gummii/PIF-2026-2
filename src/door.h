#ifndef DOOR_H
#define DOOR_H

#include <raylib.h>

typedef enum {
	DOOR_PORTAL,
	DOOR_GATE
} DoorStyle;

typedef struct {
	DoorStyle style;
	Rectangle area;
	Rectangle entry;
	bool unlocked;
	bool playerNear;
	float openT;
	float time;
} Door;

void Door_Init(Door* door, DoorStyle style, Rectangle area, Rectangle entry);
void Door_Update(Door* door, float dt, bool unlocked, Rectangle plrFeet);
bool Door_IsOpen(const Door* door);
void Door_Draw(const Door* door, const char* lockedHint);

#endif
