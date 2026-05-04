#include <cstdio>
#include "raylib.h"

#define BACKGROUND_COLOR SKYBLUE
#define CANVAS_WIDTH 1600
#define CANVAS_HEIGHT 900

#ifdef _cplusplus
extern "C" {
#endif
int main(void) {
	// init app
	InitWindow(CANVAS_WIDTH, CANVAS_HEIGHT, "First Raylib App");
	// run app
	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(BACKGROUND_COLOR);
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
