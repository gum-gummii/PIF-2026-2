#include <raylib.h>
#include <resource_dir.h>

void DrawBattleBackground() {
	int squareSize = 500;
	int x = (GetScreenWidth() / 2) - (squareSize / 2);
	int y = (GetScreenHeight() / 2) - (squareSize / 2);

	Rectangle box = { (float)x, (float)y, (float)squareSize, (float)squareSize };
	Color bgColor = {0, 0, 0, 100};

	DrawRectangle(x, y, squareSize, squareSize, bgColor);
	DrawRectangleRec(box, bgColor);
	DrawRectangleLinesEx(box, 8, GREEN);
}


int main() {
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(GetScreenWidth(), GetScreenHeight(), "ALTERNUDE");


	SearchAndSetResourceDir("resources");

	Texture wabbit = LoadTexture("wabbit_alpha.png");


	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(PINK);

		DrawText("I am the one who knocks", 200, 200, 20, WHITE);

		// draw our texture to the screen
		DrawTexture(wabbit, 400, 400, BLUE);
		DrawTexture(wabbit, 600, 400, WHITE);

		Vector2 wabPos = { 800, 400 };
		DrawTextureEx(wabbit, wabPos, 0, 2, PINK);

		DrawBattleBackground();
		
		// end the frame and get ready for the next one  (display frame, poll input, etc...)
		EndDrawing();
	}

	UnloadTexture(wabbit);
	CloseWindow();
	return 0;
}
