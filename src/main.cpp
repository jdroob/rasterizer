#include <cstdio>
#include <cmath>
#include <functional>
#include <vector>
#include "raylib.h"

#define BACKGROUND_COLOR SKYBLUE
#define CANVAS_WIDTH 1600
#define CANVAS_HEIGHT 900

#define BOUND_COLOR(colorChannel) (unsigned char)((colorChannel) > 255 ? 255 : colorChannel)

/**
 * Rasterization Notes:
 *  Let's get the line math out of the way:
 * 		- Given 2 points: P0 and P1, represent the line: P0->P1 as 
 *      P = P0 + t(P1 - P0)
 *      
 * 			where P is any point on the line P0->P1.
 * 
 *    - Decompose the above into
 * 		 x = x0 + t(x1 - x0)
 *     y = y0 + t(y1 - y0)
 * 
 *     then
 *     t = (x - x0) / (x1 - x0)
 *     => y = y0 + [(x-x0)/(x1-x0)] * (y1-y0)
 *     => y = y0 + (x-x0)[(y1-y0) / (x1-x0)]
 * 
 *    Notice 
 *     [(y1-y0) / (x1-x0)] is a constant that simply depends on the endpoints
 *     so let's call this conastant `a`.
 * 
 * 	   => y = y0 + a(x-x0)   // Note: a is really the slope of the line
 *     => y = y0 + ax - ax0
 *     => y = ax + y0-ax0
 * 
 *    Let b := y0-ax0 (since y0-ax0 is simply a constant defined by the line's endpoints)
 *    => y = ax + b.
 * 
 *   In summary, we now have our handy dandy linear formula: y = ax + b.
 *   Note that this can define any line EXCEPT vertical lines b/c that'd imply x0 == x1
 *   and therefore we'd have a div by zero. So we'll ignore those for now.
 * 
 *  So our pseudo-code for DrawLine is:
 *  
 *  DrawLine(P0, P1, Color) {
 * 	a = (y1-y0) / (x1-x0);
 *    b = (y0 - a * x0);
 *    for x in x0 to x1:
 * 			y = a * x + b;
 *      DrawLine(x, y, Color);
 *  }
 */

 struct Point_t {
	float x, y;
 };

 Color operator*(float h, const Color& color) {
	return (Color){
		BOUND_COLOR(h * color.r),
		BOUND_COLOR(h * color.g),
		BOUND_COLOR(h * color.b),
		color.a
	};
 }

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

void Interpolate(int i0, int i1, float d0, float d1, std::vector<float>& values) {
  if (i0 == i1) {
		values.push_back(d0);
		return;
	}	
	float a = (d1 - d0) / (i1 - i0);
	float d = d0;
	for (int i=i0; i<=i1; ++i) {
		values.push_back(d);
		d += a;
	}	
}

void DrawLine(Point_t P0, Point_t P1, const Color& color) {
		if (std::fabs(P1.y - P0.y) > std::fabs(P1.x - P0.x)) {
			// y must be independent var
			if (P0.y > P1.y) {
				std::swap(P0, P1);
			}
			std::vector<float> xVals;
			Interpolate(P0.y, P1.y, P0.x, P1.x, xVals);
			for (int y=P0.y; y<=P1.y; ++y) {
				PutPixel(xVals[y-P0.y], y, color);
			}
		} else {
			// x must be independent var
			if (P0.x > P1.x) {
				std::swap(P0, P1);
			}
			std::vector<float> yVals;
			Interpolate(P0.x, P1.x, P0.y, P1.y, yVals);
			for (int x=P0.x; x<=P1.x; ++x) {
				PutPixel(x, yVals[x-P0.x], color);
			}
		}
}

void FillTriangle(Point_t P0, Point_t P1, Point_t P2, const Color& color, float h0, float h1, float h2) {
	// Sort the vertices by y coordinate ascending (P0, P1, P2)
	if (P0.y > P1.y) std::swap(P0, P1);
	if (P0.y > P2.y) std::swap(P0, P2);
	if (P1.y > P2.y) std::swap(P1, P2);

	// Compute the x coordinates of the triangle edges at each y coordinate
	std::vector<float> x01; Interpolate(P0.y, P1.y, P0.x, P1.x, x01);
	std::vector<float> h01; Interpolate(P0.y, P1.y, h0, h1, h01);
	std::vector<float> x12; Interpolate(P1.y, P2.y, P1.x, P2.x, x12);
	std::vector<float> h12; Interpolate(P1.y, P2.y, h1, h2, h12);
	std::vector<float> x02; Interpolate(P0.y, P2.y, P0.x, P2.x, x02); x02.pop_back();
	std::vector<float> h02; Interpolate(P0.y, P2.y, h0, h2, h02); h02.pop_back();
	std::vector<float> x012 = x01;
	std::vector<float> h012 = h01;
	x012.insert(x012.end(), x12.begin(), x12.end());
	h012.insert(h012.end(), h12.begin(), h12.end());

	// Determine which is left and which is right
	std::vector<float> *xLeft, *xRight;
	std::vector<float> *hLeft, *hRight;
	int m = x02.size() / 2;
	if (x02[m] < x012[m]) {
		xLeft = &x02;
		hLeft = &h02;
		xRight = &x012;
		hRight = &h012;
	} else {
		xLeft = &x012;
		hLeft = &h012;
		xRight = &x02;
		hRight = &h02;
	}
	for (int y=P0.y; y<= P2.y; ++y) {
		int xL = (*xLeft)[y-P0.y];
		float hL = (*hLeft)[y-P0.y];
		int xR = (*xRight)[y-P0.y];
		float hR = (*hRight)[y-P0.y];
		std::vector<float> hSegment; Interpolate(xL, xR, hL, hR, hSegment);
		for (int x=xL; x<=xR; ++x) {
			float h = hSegment[x-xL];
			Color shadedColor = h * color;
			PutPixel(x, y, shadedColor);
		}
	}
}

void DrawWireFrameTriangle(Point_t P0, Point_t P1, Point_t P2, const Color& color) {
	DrawLine(P0, P1, color);
	DrawLine(P1, P2, color);
	DrawLine(P2, P0, color);
}

void DrawFilledTriangle(Point_t P0, Point_t P1, Point_t P2, float f0, float f1, float f2, const Color& color) {
	FillTriangle(P0, P1, P2, color, f0, f1, f2);
}

int main(void) {
	// init app
	InitWindow(CANVAS_WIDTH, CANVAS_HEIGHT, "Rasterizer");
	// run app
	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(BACKGROUND_COLOR);
		DrawFilledTriangle(
			{-200, 250},
			{200, 50},
			{20, 250},
			0.8f, 0.4f, 0.2f,
			RED
		);
		DrawWireFrameTriangle(
			{-200, 250},
			{200, 50},
			{20, 250},
			BLACK
		);
		// DrawWireFrameTriangle(
		// 	{200, -250},
		// 	{-200, -50},
		// 	{-20, -250},
		// 	BLACK
		// );
		EndDrawing();
	}
	// close app
	CloseWindow();
	return 0;
}
