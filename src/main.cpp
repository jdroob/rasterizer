#include <cstdio>
#include "raylib.h"

#define BACKGROUND_COLOR SKYBLUE
#define CANVAS_WIDTH 1600
#define CANVAS_HEIGHT 900

int boundX(int x) {
	if (x < 0) return 0;
	if (x >= CANVAS_WIDTH) return CANVAS_WIDTH - 1;
	return x;
}
int boundY(int y) {
	if (y < 0) return 0;
	if (y >= CANVAS_HEIGHT) return CANVAS_HEIGHT - 1;
	return y;
}
void PutPixel(int x, int y, const Color color) {
	// Translate from Cartesian coordinates to screen coordinates
	int Cx = boundX(x + CANVAS_WIDTH / 2);
	int Cy = boundY((-y) + CANVAS_HEIGHT / 2);
	DrawPixel(Cx, Cy, color);
}

int main(void) {
	// init app
	InitWindow(CANVAS_WIDTH, CANVAS_HEIGHT, "First Raylib App");
	// run app
	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(BACKGROUND_COLOR);
		PutPixel(0, 0, RED);
		EndDrawing();
	}
	// close app
	CloseWindow();
	return 0;
}
