#include <sprites.h>
#include <math.h>

#define DEMON_GRID 12

static const char* DEMON_SPRITE[DEMON_GRID] = {
	"..H......H..",
	"..HH....HH..",
	"..PPPPPPPP..",
	".PPPPPPPPPP.",
	".PWEPPPPEWP.",
	".PPPPPPPPPP.",
	".PPMMMMMMPP.",
	"..PPPPPPPP..",
	".DDPPPPPPDD.",
	".D.PPPPPP.D.",
	"...PP..PP...",
	"..FFF..FFF..",
};

DemonPalette Sprite_DemonPalette(void) {
	return (DemonPalette){
		.horn = {240, 220, 140, 255},
		.body = {130, 70, 190, 255},
		.dark = {80, 40, 130, 255},
		.eyeWhite = WHITE,
		.pupil = BLACK,
		.mouth = {90, 10, 30, 255},
		.feet = {40, 20, 60, 255},
	};
}

DemonPalette Sprite_FlatPalette(Color color) {
	return (DemonPalette){color, color, color, color, color, color, color};
}

static Color DemonColor(char ch, const DemonPalette* p) {
	switch (ch) {
		case 'H': return p->horn;
		case 'P': return p->body;
		case 'D': return p->dark;
		case 'W': return p->eyeWhite;
		case 'E': return p->pupil;
		case 'M': return p->mouth;
		case 'F': return p->feet;
		default:  return BLANK;
	}
}

void Sprite_DrawSlime(Vector2 p, float s, float bob, Color c, float alpha) {
	float squish = sinf(bob) * 0.12f;
	float w = s * (1.0f + squish), h = s * 0.8f * (1.0f - squish);
	DrawEllipse((int)p.x, (int)(p.y + s / 2 - 2), s * 0.5f, s * 0.15f, Fade(BLACK, 0.3f * alpha));
	DrawEllipse((int)p.x, (int)(p.y + s / 2 - h / 2), w / 2, h / 2, Fade(c, alpha));
	DrawCircle((int)(p.x - s * 0.18f), (int)(p.y + s / 2 - h * 0.6f), s * 0.08f, Fade(BLACK, alpha));
	DrawCircle((int)(p.x + s * 0.18f), (int)(p.y + s / 2 - h * 0.6f), s * 0.08f, Fade(BLACK, alpha));
}

void Sprite_DrawBat(Vector2 p, float s, float bob, Color c, float alpha) {
	float flap = sinf(bob) * s * 0.35f;
	float y = p.y + sinf(bob * 0.3f) * 4;
	Color body = Fade(c, alpha);
	DrawEllipse((int)p.x, (int)(p.y + s * 0.8f), s * 0.35f, s * 0.1f, Fade(BLACK, 0.25f * alpha));
	DrawTriangle((Vector2){p.x - s * 0.2f, y}, (Vector2){p.x - s * 0.8f, y - flap}, (Vector2){p.x - s * 0.25f, y + s * 0.2f}, body);
	DrawTriangle((Vector2){p.x + s * 0.2f, y}, (Vector2){p.x + s * 0.25f, y + s * 0.2f}, (Vector2){p.x + s * 0.8f, y - flap}, body);
	DrawCircle((int)p.x, (int)y, s * 0.32f, body);
	DrawCircle((int)(p.x - s * 0.12f), (int)(y - s * 0.05f), s * 0.07f, Fade(YELLOW, alpha));
	DrawCircle((int)(p.x + s * 0.12f), (int)(y - s * 0.05f), s * 0.07f, Fade(YELLOW, alpha));
}

void Sprite_DrawTank(Vector2 p, float s, float bob, Color c, float alpha) {
	float step = fabsf(sinf(bob)) * 3;
	Rectangle r = {p.x - s / 2, p.y - s / 2 - step, s, s};
	DrawEllipse((int)p.x, (int)(p.y + s / 2), s * 0.55f, s * 0.15f, Fade(BLACK, 0.3f * alpha));
	DrawRectangleRounded(r, 0.35f, 6, Fade(c, alpha));
	DrawRectangleRoundedLinesEx(r, 0.35f, 6, 2, Fade(BLACK, 0.5f * alpha));
	DrawRectangle((int)(p.x - s * 0.28f), (int)(p.y - s * 0.15f - step), (int)(s * 0.16f), (int)(s * 0.08f), Fade(BLACK, alpha));
	DrawRectangle((int)(p.x + s * 0.12f), (int)(p.y - s * 0.15f - step), (int)(s * 0.16f), (int)(s * 0.08f), Fade(BLACK, alpha));
}

void Sprite_DrawDemon(Vector2 center, float size, DemonPalette palette, float alpha) {
	float px = size / DEMON_GRID;
	float left = center.x - size / 2, top = center.y - size / 2;

	DrawEllipse((int)center.x, (int)(center.y + size / 2), size / 2, size * 0.09f, Fade(BLACK, 0.35f * alpha));

	for (int row = 0; row < DEMON_GRID; row++) {
		for (int col = 0; col < DEMON_GRID; col++) {
			char ch = DEMON_SPRITE[row][col];
			if (ch == '.') { continue; }
			Rectangle cell = {left + col * px, top + row * px, px + 0.5f, px + 0.5f};
			DrawRectangleRec(cell, Fade(DemonColor(ch, &palette), alpha));
		}
	}
}
