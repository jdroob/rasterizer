#include <cstdio>

#include "raylib.h"

#ifdef _cplusplus
extern "C" {
#endif
int main(void) {
	// init app
	InitWindow(800, 600, "First Raylib App");
	// run app
	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(SKYBLUE);
		DrawPixel(400, 300, RED);
		EndDrawing();
	}
	// close app
	CloseWindow();
	return 0;
}
#ifdef _cplusplus
}
#endif
