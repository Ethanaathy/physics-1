// main.cpp
/*
  Simple Raylib physics playground
  - Base class FizziksObjekt (virtual draw)
  - Derived FizziksCircle (radius, overrides draw)
  - FizziksWorld holds pointers (polymorphism)
  - Spawn circles with SPACE
  - Pairwise circle-circle overlap -> color change
  - GUI sliders for time, speed, angle, gravity Y
*/

#include "raylib.h"
#include "raymath.h"

// NOTE: include RayGUI in exactly one translation unit
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <vector>
#include <string>

// ------------------------------ Globals / constants
static const int    InitialWidth = 1280;
static const int    InitialHeight = 720;

static const unsigned int TARGET_FPS = 50;        // frames/second
static float dt = 1.0f / TARGET_FPS;            // seconds/frame (fixed)
static float timeAccum = 0.0f;                    // for displaying "T:"

// Launch controls (shown in GUI)
static float speed = 100.0f;                      // pixels/sec
static float angleDeg = 0.0f;                     // degrees (0 = +X)

// ------------------------------ Base object
struct FizziksObjekt {
    Vector2 position{ 0, 0 };
    Vector2 velocity{ 0, 0 };
    float   mass = 1.0f;                      // kg (unused here)
    std::string name = "objekt";
    Color   color = GREEN;

    virtual ~FizziksObjekt() = default;
    virtual void draw() {
        // base: nothing (derived overrides)
    }
};

// ------------------------------ Circle object (derived)
struct FizziksCircle : public FizziksObjekt {
    float radius = 15.0f; // pixels

    void draw() override {
        DrawCircleV(position, radius, color);
        // velocity vector preview
        Vector2 tip = Vector2Add(position, Vector2Scale(velocity, 0.2f));
        DrawLineEx(position, tip, 1.0f, color);
    }
};

// ------------------------------ Utility
static bool CircleCircleOverlap(FizziksCircle* a, FizziksCircle* b)
{
    float dist = Vector2Distance(a->position, b->position);
    float sumR = a->radius + b->radius;
    return dist < sumR;
}

// ------------------------------ World (polymorphic pointers)
struct FizziksWorld {
private:
    unsigned int objektCount = 0;

public:
    std::vector<FizziksObjekt*> objekts;     // store base pointers
    Vector2 accelerationGravity{ 0, 9 };     // px/sec^2 (+Y is down)

    ~FizziksWorld() {
        for (auto* p : objekts) delete p;
        objekts.clear();
    }

    void add(FizziksObjekt* newObject) {
        newObject->name = std::to_string(objektCount++);
        objekts.push_back(newObject);
    }

    // Basic kinematics update for all objects, then collisions
    void update() {
        for (size_t i = 0; i < objekts.size(); ++i) {
            FizziksObjekt* o = objekts[i];
            o->position = Vector2Add(o->position, Vector2Scale(o->velocity, dt));              // p += v*dt
            o->velocity = Vector2Add(o->velocity, Vector2Scale(accelerationGravity, dt));      // v += g*dt
        }
        checkCollisions();
    }

    // Pairwise circle/circle color toggling (correct per-frame logic)
    void checkCollisions() {
        // 1) default: all GREEN
        for (auto* base : objekts) {
            base->color = GREEN;
        }

        // 2) mark RED if overlapping (i<j, no self or duplicates)
        for (int i = 0; i < (int)objekts.size(); ++i) {
            for (int j = i + 1; j < (int)objekts.size(); ++j) {
                FizziksCircle* A = (FizziksCircle*)objekts[i];
                FizziksCircle* B = (FizziksCircle*)objekts[j];
                if (CircleCircleOverlap(A, B)) {
                    A->color = RED;
                    B->color = RED;
                }
            }
        }
    }
};

static FizziksWorld world;

// ------------------------------ Housekeeping: remove offscreen objects
static void cleanup()
{
    for (int i = 0; i < (int)world.objekts.size(); ++i) {
        FizziksObjekt* o = world.objekts[i];
        bool off =
            (o->position.y > GetScreenHeight()) || (o->position.y < 0) ||
            (o->position.x > GetScreenWidth()) || (o->position.x < 0);

        if (off) {
            delete o;
            world.objekts.erase(world.objekts.begin() + i);
            --i;
        }
    }
}

// ------------------------------ Frame update (time, input, world)
static void updateFrame()
{
    dt = 1.0f / TARGET_FPS;   // fixed dt
    timeAccum += dt;

    cleanup();
    world.update();

    // Spawn a new circle on SPACE
    if (IsKeyPressed(KEY_SPACE)) {
        auto* c = new FizziksCircle();
        c->position = { 100.0f, (float)GetScreenHeight() - 100.0f };
        c->velocity = {
            speed * cosf(angleDeg * DEG2RAD),
           -speed * sinf(angleDeg * DEG2RAD)   // negative Y is up in Raylib
        };
        c->color = GREEN;
        world.add(c);
    }
}

// ------------------------------ Frame draw (GUI + vectors + objects)
static void drawFrame()
{
    BeginDrawing();
    ClearBackground(BLACK);

    DrawText("Aathiththan Yogeswaran 101462564", 10, GetScreenHeight() - 30, 20, LIGHTGRAY);

    // GUI – sliders (C++ aggregate args are OK)
    GuiSliderBar(Rectangle{ 10, 15, 1000, 20 }, " ", TextFormat("t: %.2f", timeAccum), &timeAccum, 0.0f, 240.0f);
    GuiSliderBar(Rectangle{ 10, 40, 500, 30 }, "Speed", TextFormat("Speed: %.0f", speed), &speed, -1000.0f, 1000.0f);
    GuiSliderBar(Rectangle{ 10, 80, 500, 30 }, "Angle", TextFormat("Angle: %.0f deg", angleDeg), &angleDeg, -180.0f, 180.0f);
    GuiSliderBar(Rectangle{ 10, 120, 500, 30 }, "Gravity Y", TextFormat("Gravity Y: %.1f px/s^2", world.accelerationGravity.y),
        &world.accelerationGravity.y, -50.0f, 50.0f);

    DrawText(TextFormat("Objects: %i", (int)world.objekts.size()), 10, 160, 30, LIGHTGRAY);
    DrawText(TextFormat("T: %.2f", timeAccum), GetScreenWidth() - 140, 10, 30, LIGHTGRAY);

    // Visualize launch vector from the spawn point
    Vector2 startPos = { 100.0f, (float)GetScreenHeight() - 100.0f };
    Vector2 v = { speed * cosf(angleDeg * DEG2RAD), -speed * sinf(angleDeg * DEG2RAD) };
    DrawLineEx(startPos, Vector2Add(startPos, v), 3.0f, RED);

    // Polymorphic draw
    for (size_t i = 0; i < world.objekts.size(); ++i) {
        world.objekts[i]->draw();
    }

    EndDrawing();
}

// ------------------------------ Entry
int main()
{
    InitWindow(InitialWidth, InitialHeight, "GAME2005 Aathiththan Yogeswaran 101462564 - Week 5");
    SetTargetFPS(TARGET_FPS);

    while (!WindowShouldClose()) {
        updateFrame();
        drawFrame();
    }

    CloseWindow();
    return 0;
}
