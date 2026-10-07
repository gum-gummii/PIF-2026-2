#include <raylib.h>
#include <resource_dir.h>


int main() {
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(GetScreenWidth(), GetScreenHeight(), "ALTERNUDE");


	SearchAndSetResourceDir("resources");

	Texture wabbit = LoadTexture("wabbit_alpha.png");


	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(BLACK);

		DrawText("I am the one who knocks", 200, 200, 20, WHITE);

		// draw our texture to the screen
		DrawTexture(wabbit, 400, 400, BLUE);
		DrawTexture(wabbit, 600, 400, WHITE);

		Vector2 wabPos = { 800, 400 };
		DrawTextureEx(wabbit, wabPos, 0, 2, PINK);
		
		// end the frame and get ready for the next one  (display frame, poll input, etc...)
		EndDrawing();
	}

	UnloadTexture(wabbit);
	CloseWindow();
	return 0;
}
