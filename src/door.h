#ifndef DOOR_H
#define DOOR_H

#include <raylib.h>

typedef struct {
	Rectangle panels;
	Rectangle entry;
	bool unlocked;
	bool playerNear;
	float openT;
	float time;
} Door;

void Door_Init(Door* door, Rectangle panels, Rectangle entry);
void Door_Update(Door* door, float dt, bool unlocked, Rectangle plrFeet);
bool Door_IsEntering(const Door* door, Rectangle plrFeet);
void Door_Draw(const Door* door, const char* lockedHint);

#endif
