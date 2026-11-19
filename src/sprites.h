#ifndef SPRITES_H
#define SPRITES_H

#include <raylib.h>

typedef struct {
	Color horn;
	Color body;
	Color dark;
	Color eyeWhite;
	Color pupil;
	Color mouth;
	Color feet;
} DemonPalette;

typedef struct {
	Color horn;
	Color body;
	Color wing;
	Color belly;
	Color eye;
	Color teeth;
	Color mouth;
	Color claw;
} DragonPalette;

DemonPalette Sprite_DemonPalette(void);
DemonPalette Sprite_FlatPalette(Color color);

void Sprite_DrawSlime(Vector2 center, float size, float bob, Color color, float alpha);
void Sprite_DrawBat(Vector2 center, float size, float bob, Color color, float alpha);
void Sprite_DrawTank(Vector2 center, float size, float bob, Color color, float alpha);
void Sprite_DrawDemon(Vector2 center, float size, DemonPalette palette, float alpha);

DragonPalette Sprite_DragonPalette(void);
DragonPalette Sprite_DragonFlat(Color color);
void Sprite_DrawDragon(Vector2 center, float size, DragonPalette palette, float alpha);

#endif
