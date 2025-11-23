// main.cpp
/*
  GAME2005 – Physics mini-framework
  Week 11: Kinetic Friction on Halfspace
  - Shapes: Circle, Halfspace (plane in 2D)
  - Forces: gravity, normal, kinetic friction (F = μN)
  - Response: translate out of overlap; respect static objects (“Fix”)
  - Visuals: draw force vectors (gravity, normal, friction) plus velocity
  - GUI: ground angle, gravity Y
  - 4 spheres with different masses and coefficients of friction

  Student: Aathiththan Yogeswaran 101462564
*/

#include "raylib.h"
#include "raymath.h"

// Include RayGUI 
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <vector>
#include <string>
#include <cmath>

//   Window / timing
static const int  InitialWidth = 1280;
static const int  InitialHeight = 720;

static const unsigned int TARGET_FPS = 60;      // frames/second
static float dt = 1.0f / TARGET_FPS;     // seconds/frame (fixed)
static float timeAccum = 0.0f;                  // displayed time

// Small epsilon for separation
static const float EPS = 0.001f;

// Ground angle (Halfspace) – controlled by GUI
static float groundAngleDeg = 0.0f;             // 0 = horizontal

//   Helpers
static inline float Vector2Dot(Vector2 a, Vector2 b) { return a.x * b.x + a.y * b.y; }

//   Shape enum
enum FizziksShape
{
    CIRCLE,
    HALF_SPACE
};

//   Base object
struct FizziksObjekt {
    bool     isStatic = false;               // "Fix" when true
    Vector2  position{ 0, 0 };
    Vector2  velocity{ 0, 0 };
    float    mass = 1.0f;
    std::string name = "objekt";
    Color    color = GREEN;                  // current color
    Color    baseColor = GREEN;              // original color to restore

    virtual ~FizziksObjekt() = default;

    virtual void draw() {
        // Base draws nothing
    }

    virtual FizziksShape Shape() = 0;

    void makeStatic(bool v = true) { isStatic = v; }
};

//   Circle object
struct FizziksCircle : public FizziksObjekt {
    float radius = 18.0f;    // pixels
    float kFriction = 0.1f;     // coefficient of kinetic friction μ
    // Force vectors for drawing
    Vector2 Fgravity{ 0, 0 };
    Vector2 Fnormal{ 0, 0 };
    Vector2 Ffriction{ 0, 0 };

    void draw() override {
        // Sphere
        DrawCircleV(position, radius, Fade(color, 0.6f));
        DrawText(name.c_str(), (int)(position.x - radius), (int)(position.y - radius * 2), 12, LIGHTGRAY);

        // Velocity (red)
        Vector2 velEnd = Vector2Add(position, Vector2Scale(velocity, 0.1f));
        DrawLineEx(position, velEnd, 2.0f, RED);

        // Gravity (purple)
        Vector2 gEnd = Vector2Add(position, Vector2Scale(Fgravity, 0.02f));
        DrawLineEx(position, gEnd, 2.0f, PURPLE);

        // Normal (green)
        Vector2 nEnd = Vector2Add(position, Vector2Scale(Fnormal, 0.02f));
        DrawLineEx(position, nEnd, 2.0f, GREEN);

        // Friction (orange)
        Vector2 fEnd = Vector2Add(position, Vector2Scale(Ffriction, 0.02f));
        DrawLineEx(position, fEnd, 2.0f, ORANGE);
    }

    FizziksShape Shape() override { return CIRCLE; }
};

//   Halfspace (2D plane)
struct FizziksHalfspace : public FizziksObjekt {
private:
    float  rotationDeg = 0.0f;          // purely visual/debug
    Vector2 normal{ 0, -1 };            // unit normal (points "inside" kept half)

public:
    void setRotationDegrees(float deg) {
        rotationDeg = deg;
        // default normal is up (0,-1); rotate to get any orientation
        normal = Vector2Rotate(Vector2{ 0, -1 }, rotationDeg * DEG2RAD);
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

        // Draw infinite line: tangent is normal rotated by 90 degrees
        Vector2 tangent = Vector2Rotate(normal, PI * 0.5f);
        DrawLineEx(Vector2Add(position, Vector2Scale(tangent, -4000.0f)),
            Vector2Add(position, Vector2Scale(tangent, 4000.0f)), 1.0f, color);
    }

    FizziksShape Shape() override { return HALF_SPACE; }
};

//   Overlap tests
static bool CircleCircleOverlap(FizziksCircle* a, FizziksCircle* b)
{
    Vector2 d = Vector2Subtract(b->position, a->position);
    float dist = Vector2Length(d);
    return dist < (a->radius + b->radius);
}

// signed distance = dot( (C - P0), n ); overlap if (radius - signedD) > 0
static bool CircleHalfspaceOverlap(FizziksCircle* c, FizziksHalfspace* h)
{
    Vector2 toC = Vector2Subtract(c->position, h->position);
    float   dSign = Vector2Dot(toC, h->getNormal());    // positive = "above" plane along normal
    float   pen = c->radius - dSign;
    return pen > 0.0f;
}

//   Separation responses
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

//   World (polymorphic)

struct FizziksWorld {
private:
    unsigned int objektCount = 0;

public:
    std::vector<FizziksObjekt*> objekts;
    // gravity as acceleration (pixels/s^2), +Y down
    Vector2 accelerationGravity{ 0, 300 };

    ~FizziksWorld() {
        for (auto* p : objekts) delete p;
        objekts.clear();
    }

    void add(FizziksObjekt* obj) {
        obj->name = std::to_string(objektCount++);
        objekts.push_back(obj);
    }

    void update();

    void checkCollisions();
    void cleanupOffscreen();

    void draw() {
        for (auto* o : objekts) o->draw();
    }
};

static FizziksWorld world;
static FizziksHalfspace* gGround = nullptr;  // main Halfspace used for friction

//   World::update with forces

void FizziksWorld::update()
{
    dt = 1.0f / TARGET_FPS;
    timeAccum += dt;

    // restore colors every frame
    for (auto* o : objekts) o->color = o->baseColor;

    // --- Force-based integration for circles ---
    for (auto* o : objekts) {
        if (o->isStatic) continue;

        if (o->Shape() == CIRCLE) {
            auto* c = (FizziksCircle*)o;

            // Gravity force
            Vector2 Fg = Vector2Scale(accelerationGravity, c->mass);
            Vector2 Fn{ 0,0 };
            Vector2 Ff{ 0,0 };

            if (gGround) {
                Vector2 n = gGround->getNormal();

                // Check if close enough to be considered in contact
                Vector2 toC = Vector2Subtract(c->position, gGround->position);
                float   dSign = Vector2Dot(toC, n);
                float   pen = c->radius - dSign;

                if (pen >= -1.0f) { // slightly above still counts as resting on surface
                    // Decompose gravity into normal + tangential components
                    float gNmag = Vector2Dot(accelerationGravity, n);
                    Vector2 gN = Vector2Scale(n, gNmag);
                    Vector2 gT = Vector2Subtract(accelerationGravity, gN);

                    // Normal force cancels component of gravity into plane
                    Fn = Vector2Scale(n, -gNmag * c->mass);    // opposite direction
                    float Nmag = Vector2Length(Fn);

                    // Kinetic friction magnitude μN, opposite tangential gravity
                    float gTlen = Vector2Length(gT);
                    if (gTlen > 0.0001f && Nmag > 0.0f) {
                        Vector2 dirOppose = Vector2Negate(Vector2Scale(gT, 1.0f / gTlen));
                        float FfMag = c->kFriction * Nmag;
                        Ff = Vector2Scale(dirOppose, FfMag);
                    }
                }
            }

            // Net force and acceleration
            Vector2 Fnet = Vector2Add(Fg, Vector2Add(Fn, Ff));
            Vector2 acc = Vector2Scale(Fnet, 1.0f / c->mass);

            // Integrate
            c->velocity = Vector2Add(c->velocity, Vector2Scale(acc, dt));
            c->position = Vector2Add(c->position, Vector2Scale(c->velocity, dt));

            // store for drawing
            c->Fgravity = Fg;
            c->Fnormal = Fn;
            c->Ffriction = Ff;
        }
        else {
            // default integration for any other dynamic objects
            o->position = Vector2Add(o->position, Vector2Scale(o->velocity, dt));
            o->velocity = Vector2Add(o->velocity, Vector2Scale(accelerationGravity, dt));
        }
    }

    checkCollisions();
    cleanupOffscreen();
}

void FizziksWorld::checkCollisions()
{
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

void FizziksWorld::cleanupOffscreen()
{
    for (int i = 0; i < (int)objekts.size(); ++i) {
        FizziksObjekt* o = objekts[i];
        if (o->Shape() == HALF_SPACE) continue;

        bool off =
            (o->position.y > GetScreenHeight() + 300) || (o->position.y < -300) ||
            (o->position.x > GetScreenWidth() + 300) || (o->position.x < -300);

        if (off) {
            delete o;
            objekts.erase(objekts.begin() + i);
            --i;
        }
    }
}

//   Setup 4 spheres

static void SpawnFrictionSpheres()
{
    float yStart = 200.0f;
    float xStart = 350.0f;
    float spacing = 100.0f;

    // Red    – 2 kg, μ = 0.1
    {
        auto* c = new FizziksCircle();
        c->position = { xStart + 0 * spacing, yStart };
        c->velocity = { 0, 0 };
        c->radius = 18.0f;
        c->mass = 2.0f;
        c->kFriction = 0.1f;
        c->baseColor = RED;  c->color = RED;
        world.add(c);
    }
    // Green  – 2 kg, μ = 0.8
    {
        auto* c = new FizziksCircle();
        c->position = { xStart + 1 * spacing, yStart };
        c->velocity = { 0, 0 };
        c->radius = 18.0f;
        c->mass = 2.0f;
        c->kFriction = 0.8f;
        c->baseColor = GREEN;  c->color = GREEN;
        world.add(c);
    }
    // Blue   – 8 kg, μ = 0.1
    {
        auto* c = new FizziksCircle();
        c->position = { xStart + 2 * spacing, yStart };
        c->velocity = { 0, 0 };
        c->radius = 18.0f;
        c->mass = 8.0f;
        c->kFriction = 0.1f;
        c->baseColor = BLUE;  c->color = BLUE;
        world.add(c);
    }
    // Yellow – 8 kg, μ = 0.8
    {
        auto* c = new FizziksCircle();
        c->position = { xStart + 3 * spacing, yStart };
        c->velocity = { 0, 0 };
        c->radius = 18.0f;
        c->mass = 8.0f;
        c->kFriction = 0.8f;
        c->baseColor = YELLOW;  c->color = YELLOW;
        world.add(c);
    }
}

//   Per-frame draw

static void drawFrame()
{
    BeginDrawing();
    ClearBackground(BLACK);

    // Header/footer
    DrawText("Aathiththan Yogeswaran 101462564", 10, GetScreenHeight() - 26, 20, LIGHTGRAY);
    DrawText(TextFormat("Objects: %i", (int)world.objekts.size()), 10, 10, 20, LIGHTGRAY);

    // GUI – sliders (same look as previous labs)
    GuiSliderBar(Rectangle{ 10, 40, 500, 26 }, "Ground angle",
        TextFormat("%.1f deg", groundAngleDeg), &groundAngleDeg, -45.0f, 45.0f);

    GuiSliderBar(Rectangle{ 10, 72, 500, 26 }, "GravityY",
        TextFormat("%.0f", world.accelerationGravity.y),
        &world.accelerationGravity.y, 0.0f, 1000.0f);

    // Color legend
    DrawText("Vectors: RED = velocity, PURPLE = gravity, GREEN = normal, ORANGE = friction",
        10, 110, 18, LIGHTGRAY);

    world.draw();

    EndDrawing();
}

//       Entry
int main()
{
    InitWindow(InitialWidth, InitialHeight, "GAME2005 – Lab 6: Kinetic Friction on Halfspace");
    SetTargetFPS(TARGET_FPS);

    // --- Single adjustable Halfspace (ground) ---
    {
        auto* g0 = new FizziksHalfspace();
        g0->position = { 640, 540 };   // roughly center-bottom
        g0->setRotationDegrees(groundAngleDeg);
        g0->baseColor = GRAY;
        g0->color = GRAY;
        g0->makeStatic(true);
        world.add(g0);
        gGround = g0;                   // store pointer for forces
    }

    // 4 spheres with different mass/μ
    SpawnFrictionSpheres();

    while (!WindowShouldClose()) {
        // Update ground rotation each frame from slider
        if (gGround) gGround->setRotationDegrees(groundAngleDeg);

        world.update();
        drawFrame();
    }

    CloseWindow();
    return 0;
}
