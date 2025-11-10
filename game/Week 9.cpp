// main.cpp
/*
  GAME2005 – Physics mini-framework
  Week 9: Collision Response (Separation)
  - Shapes: Circle, Halfspace (plane in 2D)
  - Overlap tests: circle–circle, circle–halfspace
  - Response: translate out of overlap; respect static objects (“Fix”)
  - Visuals: objects turn RED while overlapping; restore to baseColor each frame
  - GUI: time accumulator, launch speed/angle, gravity Y
  - Spawn circles with SPACE

  Student: Aathiththan Yogeswaran 101462564
*/

#include "raylib.h"
#include "raymath.h"

// Include RayGUI in exactly one translation unit
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <vector>
#include <string>
#include <cmath>

// ----------------------------------------------------- Window / timing
static const int  InitialWidth = 1280;
static const int  InitialHeight = 720;

static const unsigned int TARGET_FPS = 50;  // frames/second
static float dt = 1.0f / TARGET_FPS; // seconds/frame (fixed)
static float timeAccum = 0.0f;              // displayed time

// Launch controls (GUI)
static float speed = 300.0f;             // pixels/sec
static float angleDeg = 60.0f;              // degrees (0 = +X)

// Small epsilon for separation
static const float EPS = 0.001f;

// ----------------------------------------------------- Helpers
static inline float Vector2Dot(Vector2 a, Vector2 b) { return a.x * b.x + a.y * b.y; }

// ----------------------------------------------------- Shape enum
enum FizziksShape
{
    CIRCLE,
    HALF_SPACE
};

// ----------------------------------------------------- Base object
struct FizziksObjekt {
    bool     isStatic = false;               // "Fix" when true
    Vector2  position{ 0, 0 };
    Vector2  velocity{ 0, 0 };
    float    mass = 1.0f;                // (not used here)
    std::string name = "objekt";
    Color    color = GREEN;               // current color
    Color    baseColor = GREEN;               // original color to restore

    virtual ~FizziksObjekt() = default;

    // Draw hook
    virtual void draw() {
        // Base draws nothing
    }

    // Pure virtual: each child reports its shape
    virtual FizziksShape Shape() = 0;

    // Utility to fix/unfix objects
    void makeStatic(bool v = true) { isStatic = v; }
};

// ----------------------------------------------------- Circle object
struct FizziksCircle : public FizziksObjekt {
    float radius = 18.0f; // pixels

    void draw() override {
        DrawCircleV(position, radius, color);
        // Name + velocity vector (for fun)
        DrawText(name.c_str(), (int)(position.x - radius), (int)(position.y - radius * 2), 12, LIGHTGRAY);
        DrawLineEx(position, Vector2Add(position, velocity), 1.0f, color);
    }

    FizziksShape Shape() override { return CIRCLE; }
};

// ----------------------------------------------------- Halfspace (2D plane)
struct FizziksHalfspace : public FizziksObjekt {
private:
    // position = an arbitrary point on the infinite line
    float  rotationDeg = 0.0f;          // visual/debug
    Vector2 normal{ 0, -1 };       // unit normal (points "inside" kept half)

public:
    void setRotationDegrees(float deg) {
        rotationDeg = deg;
        // default normal is up (0,-1); rotate to get any orientation
        normal = Vector2Rotate(Vector2{ 0, -1 }, rotationDeg * DEG2RAD);
        // normalize to be safe
        float len = Vector2Length(normal);
        if (len > 0) normal = Vector2Scale(normal, 1.0f / len);
    }

    float  getRotation() const { return rotationDeg; }
    Vector2 getNormal()  const { return normal; }

    void draw() override {
        // Mark a point on line
        DrawCircleV(position, 6.0f, color);

        // Draw normal
        DrawLineEx(position, Vector2Add(position, Vector2Scale(normal, 40.0f)), 2.0f, color);

        // Draw infinite line (visual): a vector parallel to the surface is normal rotated by 90 degrees
        Vector2 tangent = Vector2Rotate(normal, PI * 0.5f);
        DrawLineEx(Vector2Add(position, Vector2Scale(tangent, -4000.0f)),
            Vector2Add(position, Vector2Scale(tangent, 4000.0f)), 1.0f, color);
    }

    FizziksShape Shape() override { return HALF_SPACE; }
};

// ----------------------------------------------------- Overlap tests
static bool CircleCircleOverlap(FizziksCircle* a, FizziksCircle* b)
{
    Vector2 d = Vector2Subtract(b->position, a->position);
    float dist = Vector2Length(d);
    return dist < (a->radius + b->radius);
}

// returns true if circle center is within radius distance of the plane boundary
// signed distance = dot( (C - P0), n )  ; overlap if (radius - signedD) > 0
static bool CircleHalfspaceOverlap(FizziksCircle* c, FizziksHalfspace* h)
{
    Vector2 toC = Vector2Subtract(c->position, h->position);
    float   dSign = Vector2Dot(toC, h->getNormal());    // positive = "above" plane along normal
    float   pen = c->radius - dSign;
    return pen > 0.0f;
}

// ----------------------------------------------------- Separation responses
static void SeparateCircleCircle(FizziksCircle* a, FizziksCircle* b)
{
    Vector2 ab = Vector2Subtract(b->position, a->position);
    float d = Vector2Length(ab);
    if (d <= 0.0f) { ab = Vector2{ 1,0 }; d = 1.0f; }         // degenerate

    float target = a->radius + b->radius;
    float pen = target - d;
    if (pen <= 0.0f) return;

    Vector2 n = Vector2Scale(ab, 1.0f / d);                 // normalized from A->B

    // Move the dynamic ones. If one is static, move only the other.
    float moveA = a->isStatic ? 0.0f : 1.0f;
    float moveB = b->isStatic ? 0.0f : 1.0f;
    float sum = moveA + moveB;
    if (sum <= 0.0f) return;                                 // both static

    float kA = moveA / sum;
    float kB = moveB / sum;

    Vector2 corr = Vector2Scale(n, pen + EPS);
    a->position = Vector2Subtract(a->position, Vector2Scale(corr, kA));
    b->position = Vector2Add(b->position, Vector2Scale(corr, kB));

    // Remove inward normal velocity to keep them from re-penetrating
    float vAn = Vector2Dot(a->velocity, n);
    float vBn = Vector2Dot(b->velocity, n);
    if (!a->isStatic && vAn > 0) a->velocity = Vector2Subtract(a->velocity, Vector2Scale(n, vAn));
    if (!b->isStatic && vBn < 0) b->velocity = Vector2Subtract(b->velocity, Vector2Scale(n, vBn));
}

static void SeparateCircleHalfspace(FizziksCircle* c, FizziksHalfspace* h)
{
    Vector2 toC = Vector2Subtract(c->position, h->position);
    float   dSign = Vector2Dot(toC, h->getNormal());
    float   pen = c->radius - dSign;
    if (pen <= 0.0f) return;

    Vector2 push = Vector2Scale(h->getNormal(), pen + EPS);
    if (!c->isStatic) {
        c->position = Vector2Add(c->position, push);

        // Zero inward normal velocity (into plane = negative along normal)
        float vn = Vector2Dot(c->velocity, h->getNormal());
        if (vn < 0) c->velocity = Vector2Subtract(c->velocity, Vector2Scale(h->getNormal(), vn));
    }
}

// ----------------------------------------------------- World (polymorphic)
struct FizziksWorld {
private:
    unsigned int objektCount = 0;

public:
    std::vector<FizziksObjekt*> objekts;
    Vector2 accelerationGravity{ 0, 300 };    // px/s^2 (tune via GUI if you want)

    ~FizziksWorld() {
        for (auto* p : objekts) delete p;
        objekts.clear();
    }

    void add(FizziksObjekt* obj) {
        obj->name = std::to_string(objektCount++);
        objekts.push_back(obj);
    }

    void update() {
        // restore colors every frame
        for (auto* o : objekts) o->color = o->baseColor;

        // Kinematics for dynamic bodies
        for (auto* o : objekts) {
            if (o->isStatic) continue;
            o->position = Vector2Add(o->position, Vector2Scale(o->velocity, dt));
            o->velocity = Vector2Add(o->velocity, Vector2Scale(accelerationGravity, dt));
        }

        checkCollisions();
        cleanupOffscreen();
    }

    void checkCollisions() {
        for (int i = 0; i < (int)objekts.size(); ++i) {
            for (int j = i + 1; j < (int)objekts.size(); ++j) {
                FizziksObjekt* A = objekts[i];
                FizziksObjekt* B = objekts[j];

                FizziksShape sA = A->Shape();
                FizziksShape sB = B->Shape();

                bool overlap = false;

                if (sA == CIRCLE && sB == CIRCLE) {
                    auto* a = (FizziksCircle*)A, * b = (FizziksCircle*)B;
                    overlap = CircleCircleOverlap(a, b);
                    if (overlap) {
                        A->color = RED; B->color = RED;
                        SeparateCircleCircle(a, b);
                    }
                }
                else if (sA == CIRCLE && sB == HALF_SPACE) {
                    auto* c = (FizziksCircle*)A; auto* h = (FizziksHalfspace*)B;
                    overlap = CircleHalfspaceOverlap(c, h);
                    if (overlap) {
                        A->color = RED; B->color = RED;
                        SeparateCircleHalfspace(c, h);
                    }
                }
                else if (sA == HALF_SPACE && sB == CIRCLE) {
                    auto* h = (FizziksHalfspace*)A; auto* c = (FizziksCircle*)B;
                    overlap = CircleHalfspaceOverlap(c, h);
                    if (overlap) {
                        A->color = RED; B->color = RED;
                        SeparateCircleHalfspace(c, h);
                    }
                }
            }
        }
    }

    // remove objects that fly far away (keeps demo tidy)
    void cleanupOffscreen() {
        for (int i = 0; i < (int)objekts.size(); ++i) {
            FizziksObjekt* o = objekts[i];
            // keep halfspaces always
            if (o->Shape() == HALF_SPACE) continue;

            bool off =
                (o->position.y > GetScreenHeight() + 200) || (o->position.y < -200) ||
                (o->position.x > GetScreenWidth() + 200) || (o->position.x < -200);

            if (off) {
                delete o;
                objekts.erase(objekts.begin() + i);
                --i;
            }
        }
    }

    void draw() {
        for (auto* o : objekts) o->draw();
    }
};

static FizziksWorld world;

// ----------------------------------------------------- Per-frame update/draw
static void updateFrame()
{
    dt = 1.0f / TARGET_FPS;
    timeAccum += dt;

    // Spawn a new circle on SPACE
    if (IsKeyPressed(KEY_SPACE)) {
        auto* c = new FizziksCircle();
        c->position = { 100.0f, (float)GetScreenHeight() - 120.0f };
        c->velocity = {
            speed * cosf(angleDeg * DEG2RAD),
           -speed * sinf(angleDeg * DEG2RAD)   // up is negative Y
        };
        c->baseColor = GREEN;
        c->color = GREEN;
        world.add(c);
    }

    world.update();
}

static void drawFrame()
{
    BeginDrawing();
    ClearBackground(BLACK);

    // Header/footer
    DrawText("Aathiththan Yogeswaran 101462564", 10, GetScreenHeight() - 26, 20, LIGHTGRAY);
    DrawText(TextFormat("Objects: %i", (int)world.objekts.size()), 10, 10, 20, LIGHTGRAY);

    // GUI – sliders
    GuiSliderBar(Rectangle{ 10, 40, 500, 26 }, "Speed", TextFormat("%.0f px/s", speed), &speed, 0.0f, 1000.0f);
    GuiSliderBar(Rectangle{ 10, 72, 500, 26 }, "Angle", TextFormat("%.0f deg", angleDeg), &angleDeg, -180.0f, 180.0f);
    GuiSliderBar(Rectangle{ 10, 104, 500, 26 }, "GravityY", TextFormat("%.0f", world.accelerationGravity.y),
        &world.accelerationGravity.y, -1000.0f, 1000.0f);

    // Visualize launch vector
    Vector2 startPos = { 100.0f, (float)GetScreenHeight() - 120.0f };
    Vector2 v = { speed * cosf(angleDeg * DEG2RAD), -speed * sinf(angleDeg * DEG2RAD) };
    DrawLineEx(startPos, Vector2Add(startPos, v), 3.0f, RED);

    world.draw();

    EndDrawing();
}

// ----------------------------------------------------- Entry
int main()
{
    InitWindow(InitialWidth, InitialHeight, "GAME2005 – Week 9: Collision Response (Separation)");
    SetTargetFPS(TARGET_FPS);

    // --- Demo halfspaces (fixed) ---
    {
        auto* g0 = new FizziksHalfspace();
        g0->position = { 400, 540 };
        g0->setRotationDegrees(0);       // horizontal ground
        g0->baseColor = GRAY; g0->color = GRAY;
        g0->makeStatic(true);
        world.add(g0);

        auto* g1 = new FizziksHalfspace();
        g1->position = { 800, 560 };
        g1->setRotationDegrees(25);
        g1->baseColor = GRAY; g1->color = GRAY;
        g1->makeStatic(true);
        world.add(g1);

        auto* g2 = new FizziksHalfspace();
        g2->position = { 220, 600 };
        g2->setRotationDegrees(-30);
        g2->baseColor = GRAY; g2->color = GRAY;
        g2->makeStatic(true);
        world.add(g2);
    }

    while (!WindowShouldClose()) {
        updateFrame();
        drawFrame();
    }

    CloseWindow();
    return 0;
}
