#include <cstdio>
#include <cmath>
#include <functional>
#include <vector>
#include "raylib.h"

#define BACKGROUND_COLOR SKYBLUE
#define CANVAS_WIDTH 1600.f
#define CANVAS_HEIGHT 900.f
#define ASPECT_RATIO (CANVAS_WIDTH / CANVAS_HEIGHT)

// SUBTLE BUT IMPORTANT:
// (CANVAS_WIDTH / CANVAS_HEIGHT) != (VIEWPORT_WIDTH / VIEWPORT_HEIGHT)
// This is why in perspective projection, x` = x * (1 / ASPECT_RATIO) * (d / z)
#define VIEWPORT_WIDTH 2.f
#define VIEWPORT_HEIGHT (2.f * (1 / ASPECT_RATIO))
#define D 1

// inputs
#define STEP 0.05

#define BOUND_COLOR(colorChannel) (unsigned char)((colorChannel) > 255 ? 255 : colorChannel)
#define VIEWPORT2CANVAS_X(x) ((x) * (CANVAS_WIDTH / VIEWPORT_WIDTH))
#define VIEWPORT2CANVAS_Y(y) ((y) * (CANVAS_HEIGHT / VIEWPORT_HEIGHT))

// Translate from Cartesian coordinates to screen coordinates
#define CANVAS2SCREEN_X(x) ((x) + CANVAS_WIDTH / 2)
#define CANVAS2SCREEN_Y(y) (-(y) + CANVAS_HEIGHT / 2)

// Translate trig inputs to radians
#define DEG2RADS(x) (x * (PI / 180.f))

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
		float x, y, z;
		Point_t operator-() {
			return {-x, -y, -z};	
		}
 };
 typedef Point_t Vec3_t;

 struct Vec4_t { 
	float x, y, z, w;
	float& operator[](size_t idx) {
    if (idx == 0) return x;
    if (idx == 1) return y;
    if (idx == 2) return z;
    return w;
	}
};

 struct Vertex_t {
		Point_t location;
		float hue;
		Vertex_t operator+(const Vec3_t& rhs) {
			return {
					{
						location.x + rhs.x, 
						location.y + rhs.y, 
						location.z + rhs.z
					}, 
					hue
				};
		}
		Vertex_t operator*(const float rhs) {
			return {
					{
						location.x * rhs, 
						location.y * rhs, 
						location.z * rhs
					}, 
					hue
				};
		}
		void operator=(const Vec4_t& rhs) {
			location.x = rhs.x;
			location.y = rhs.y;
			location.z = rhs.z;
		}
 };

 Point_t viewportToCanvas(const float& x, const float& y) {
	/**
	 * Scale viewport coordinates to canvas coordinates.
	 */
	return {
		VIEWPORT2CANVAS_X(x),
		VIEWPORT2CANVAS_Y(y),
		D  // viewport is assumed to sit parallel to xy plane at z=1
	};
 }

 Point_t projectVertex(const Vertex_t& V) {
	/**
	 * Project vertex from scene
	 * onto 2D viewport.
	 * 
	 * P`.x = P.x * (1 / ASPECT_RATIO) * (D / P.z)
	 * P`.y = P.y * (D / P.z)
	 * P`.z = D
	 * 
	 * Then, scale 2D (technically 3D but projected onto viewport) coordinates from 
	 * viewport to canvas by calling
	 * viewportToCanvas().
	 * 
	 * Returns canvas coordinates of vertex.
	 */
	const Point_t& P = V.location;
	return viewportToCanvas(
		// (P.x * (1 / ASPECT_RATIO) * (D / P.z)), 
		(P.x * (D / P.z)), 
		(P.y * (D / P.z))
	);
 }

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
void PutPixel(int Cx, int Cy, const Color color) {
	int Sx = boundX(CANVAS2SCREEN_X(Cx));
	int Sy = boundY(CANVAS2SCREEN_Y(Cy));
	DrawPixel(Sx, Sy, color);
}

void Interpolate(float i0, float i1, float d0, float d1, std::vector<float>& values) {
  if (i0 == i1) {
		values.push_back(d0);
		return;
	}	
	float a = (d1 - d0) / (i1 - i0);
	float d = d0;
	for (int i=i0; i<=i1; i+=1.f) {
		values.push_back(d);
		d += a;
	}	
}

void DrawLine(Point_t P0, Point_t P1, const Color& color) {
		/**
		 * Assumes canvas coordinate inputs.
		 */
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

void FillTriangle(Vertex_t V0, Vertex_t V1, Vertex_t V2, const Color& color) {
	/**
	 * Assumes canvas coordinate inputs.
	 */
	// Sort the vertices by y coordinate ascending (P0, P1, P2)
	Point_t P0 = V0.location; float h0 = V0.hue;
	Point_t P1 = V1.location; float h1 = V1.hue;
	Point_t P2 = V2.location; float h2 = V2.hue;
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
	for (float y=P0.y; y<= P2.y; y+=1.f) {
		float xL = (*xLeft)[y-P0.y];
		float hL = (*hLeft)[y-P0.y];
		float xR = (*xRight)[y-P0.y];
		float hR = (*hRight)[y-P0.y];
		std::vector<float> hSegment; Interpolate(xL, xR, hL, hR, hSegment);
		for (float x=xL; x<=xR; x+=1.f) {
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

void DrawFilledTriangle(Vertex_t V0, Vertex_t V1, Vertex_t V2, const Color& color) {
	FillTriangle(V0, V1, V2, color);
}

struct Triangle_t {
	unsigned V0;
	unsigned V1;
	unsigned V2;
	Color color;
};

struct Shape3D_t {
	std::vector<Vertex_t> vertices;
	std::vector<Triangle_t> triangles;
};

enum Vertex_e : int {
	_A,
	_B,
	_C,
	_D,
	_E,
	_F,
	_G,
	_H
};

struct Cube_t : Shape3D_t {
public:
	Cube_t(
		Vertex_t frontTopLeft,
		Vertex_t frontTopRight,
		Vertex_t frontBottomRight,
		Vertex_t frontBottomLeft,
		Vertex_t backTopLeft,
		Vertex_t backTopRight,
		Vertex_t backBottomRight,
		Vertex_t backBottomLeft
	) 
	{
		vertices.reserve(8);  // cube has 8 vertices
		vertices.push_back(frontTopLeft);
		vertices.push_back(frontTopRight);
		vertices.push_back(frontBottomRight);
		vertices.push_back(frontBottomLeft);
		vertices.push_back(backTopLeft);
		vertices.push_back(backTopRight);
		vertices.push_back(backBottomRight);
		vertices.push_back(backBottomLeft);

		triangles.reserve(12);  // cube has 12 triangles (2 triangles per face)
		triangles.push_back({_A, _B, _C, RED});
		triangles.push_back({_A, _C, _D, RED});
		triangles.push_back({_E, _A, _D, GREEN});
		triangles.push_back({_E, _D, _H, GREEN});
		triangles.push_back({_F, _E, _H, BLUE});
		triangles.push_back({_F, _H, _G, BLUE});
		triangles.push_back({_B, _F, _G, YELLOW});
		triangles.push_back({_B, _G, _C, YELLOW});
		triangles.push_back({_E, _F, _B, PURPLE});
		triangles.push_back({_E, _B, _A, PURPLE});
		triangles.push_back({_C, _G, _H, MAGENTA});
		triangles.push_back({_C, _H, _D, MAGENTA});
	}
};

struct Entity_t {
	Shape3D_t *shape;
	Point_t position;
	float angleY;
	float scale;
};

struct Scene_t {
	size_t nEntities;
	float sceneRotationAngle;
	Vec3_t sceneTranslation;
	Entity_t *entities;
};

//========================
// 8 vertices of model cube
Vertex_t vA_front = {{-1, 1, -1}, 1.f};
Vertex_t vB_front = {{1, 1, -1}, 1.f};
Vertex_t vC_front = {{1, -1, -1}, 1.f};
Vertex_t vD_front = {{-1, -1, -1}, 1.f};
Vertex_t vA_back = {{-1, 1, 1}, 1.f};
Vertex_t vB_back = {{1, 1, 1}, 1.f};
Vertex_t vC_back = {{1, -1, 1}, 1.f};
Vertex_t vD_back = {{-1, -1, 1}, 1.f};

// declare single cube model (assumed to be located at origin)
Cube_t cube(
	vA_front, vB_front, vC_front, vD_front,
	vA_back, vB_back, vC_back, vD_back
);

// use cube model to draw 4 concrete cubes at 4 different locations
Entity_t entities[] = {
		{&cube, {-1.5, 1, 7}, DEG2RADS(15.f), 1.5f},
		// {&cube, {-1.5, -1, 7}, DEG2RADS(-15.f)},
		// {&cube, {1.5, 1, 7}, DEG2RADS(30.f)},
		{&cube, {1.5, -1, 7}, DEG2RADS(-30.f), 0.5f}
};

Scene_t scene = 
{
	.nEntities = sizeof(entities) / sizeof(entities[0]),
	.sceneRotationAngle = DEG2RADS(-10),
	.sceneTranslation = { -1.f , -1.f, -1.f },
	.entities = entities
};

struct mat4x4 {
	float matrix[4][4];
};

//========================

static void mv_mul(const float M[4][4], Vertex_t V, Vec4_t *V_prime) {
	Vec4_t I = { V.location.x, V.location.y, V.location.z, 1 };
	// initialize output to zero
	for (int i = 0; i < 4; ++i) {
		(*V_prime)[i] = 0.0f;
	}

	// perform matrix-vector multiplication: V_prime = M * I
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			(*V_prime)[i] += M[i][j] * I[j];
		}
	}
}

void scale(Vertex_t& vertex, float scalar) {
	vertex = vertex * scalar;
}

void rotateY(Vertex_t& vertex, float angle) {
		/**
		 * Rotate x, z coordinates about the y-axis.
		 * 
		 * x` = x*cos(theta) + z*sin(theta);
		 * z` = -x*sin(theta) + z*cos(theta);
		 * 
		 * NOTE: this is equivalent to (and should be thought of as)
		 * multiplication of a 3d vector by a 3x3 rotation matrix
		 * 
		 * Ry = {
		 * 				{ cosTheta, 0,  sinTheta },
		 * 				{        0, 1,         0 },
		 * 				{ -sinTheta, 0, cosTheta },
		 * 			}
		 */
		float mat4x4[][4] = 
		{
			{  (float)cos(angle), 0, (float)sin(angle), 0 },
			{           0, 1,  				  0, 0 },
			{ -(float)sin(angle), 0,  (float)cos(angle), 0 },
			{           0, 0,           0, 1 }
		};
		Vec4_t V_prime;
		mv_mul(mat4x4, { vertex.location, 1}, &V_prime);
		// float x = vertex.location.x;
		// float z = vertex.location.z;
		// vertex.location.x = x * cos(angle) + z * sin(angle);
		// vertex.location.z = -x * sin(angle) + z * cos(angle);
		vertex = V_prime;
}

void translate(Vertex_t& vertex, Vec3_t translation) {
	vertex = vertex + translation;
}

void handleInput(Scene_t& scene) {
	if (IsKeyDown(KEY_W)) {
		scene.sceneTranslation.z += STEP;
	}
	if (IsKeyDown(KEY_S)) {
		scene.sceneTranslation.z -= STEP;
	}
	if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
		scene.sceneTranslation.x += STEP;
	}
	if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
		scene.sceneTranslation.x -= STEP;
	}
	if (IsKeyDown(KEY_UP)) {
		scene.sceneTranslation.y += STEP;
	}
	if (IsKeyDown(KEY_DOWN)) {
		scene.sceneTranslation.y -= STEP;
	}
	// if (IsKeyDown(KEY_SPACE)) {
	// 	scene = scene;
	// }
}

void applyInstanceTransformations(Vertex_t& vertex, Entity_t *entity) {
	// instance transforms
	scale(vertex, entity->scale);
	rotateY(vertex, entity->angleY);
	translate(vertex, entity->position);
}

void applyCameraTransformations(Vertex_t& vertex, Scene_t& scene) {
	// camera transforms = opposite(instance transforms)
	translate(vertex, -scene.sceneTranslation);
	rotateY(vertex, -scene.sceneRotationAngle);
}

void renderTriangle(Triangle_t triangle, const std::vector<Vertex_t>& projected) {
		DrawWireFrameTriangle(
			projected[triangle.V0].location,
			projected[triangle.V1].location,
			projected[triangle.V2].location,
			triangle.color
		);
}

void renderEntity(unsigned entityIdx, Scene_t& scene) {
		Entity_t entity = scene.entities[entityIdx];
		std::vector<Vertex_t> projected(entity.shape->vertices.size());
		Vertex_t vertex;
		for (size_t i=0; i<entity.shape->vertices.size(); ++i) {
			vertex = entity.shape->vertices[i];

			// // scale
			// vertex = vertex * entity->scale;
			// // rotate about y-axis
			// rotateY(vertex, entity->angleY);
			// // translation
			// vertex = vertex + entity->position;
			applyInstanceTransformations(vertex, &entity);
			applyCameraTransformations(vertex, scene);

			projected[i].location = projectVertex(vertex);
			projected[i].hue = entity.shape->vertices[i].hue;
		}
		for (size_t i=0; i<entity.shape->triangles.size(); ++i) {
			renderTriangle(entity.shape->triangles[i], projected);
		}
}

void renderScene(Scene_t& scene) {
		for (size_t i=0; i<scene.nEntities; ++i) {
			renderEntity(i, scene);
		}
}

int main(void) {
	// init app
	InitWindow(CANVAS_WIDTH, CANVAS_HEIGHT, "Rasterizer");
	// run app

	// 3 vertices of a triangle
	// Note: this triangle uses canvas coordinates; D is meaningless
	// Vertex_t V0 = {{-200, 250, D}, 0.8f};
	// Vertex_t V1 = {{200, 50, D}, 0.4f};
	// Vertex_t V2 = {{20, 250, D}, 0.2f};

	


	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(BACKGROUND_COLOR);
		// // edges connecting front vertices
		// DrawLine(projectVertex(vA_front), projectVertex(vB_front), BLUE);
		// DrawLine(projectVertex(vB_front), projectVertex(vC_front), BLUE);
		// DrawLine(projectVertex(vC_front), projectVertex(vD_front), BLUE);
		// DrawLine(projectVertex(vD_front), projectVertex(vA_front), BLUE);
		// // edges connecting back vertices
		// DrawLine(projectVertex(vA_back), projectVertex(vB_back), RED);
		// DrawLine(projectVertex(vB_back), projectVertex(vC_back), RED);
		// DrawLine(projectVertex(vC_back), projectVertex(vD_back), RED);
		// DrawLine(projectVertex(vD_back), projectVertex(vA_back), RED);
		// // front to back edges
		// DrawLine(projectVertex(vA_front), projectVertex(vA_back), GREEN);
		// DrawLine(projectVertex(vB_front), projectVertex(vB_back), GREEN);
		// DrawLine(projectVertex(vC_front), projectVertex(vC_back), GREEN);
		// DrawLine(projectVertex(vD_front), projectVertex(vD_back), GREEN);

		// DrawFilledTriangle(V0, V1, V2, RED);

		// DrawWireFrameTriangle(
		// 	reinterpret_cast<const Point_t&>(V0), 
		// 	reinterpret_cast<const Point_t&>(V1), 
		// 	reinterpret_cast<const Point_t&>(V2), 
		// 	BLACK
		// );

		handleInput(scene);
		renderScene(scene);

		EndDrawing();
	}
	// close app
	CloseWindow();
	return 0;
}

