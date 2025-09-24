#include "raylib.h"
#include "raymath.h"


#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "game.h"

//  Simulation Types

struct PhysicsBody {
    Vector2 pos;     // position
    Vector2 vel;     // velocity
    float   drag;    // not used this week (future use)
    float   mass;    // not used yet
    bool    active;  // integrate only if active
};

struct PhysicsSimulation {
    float   deltaTime;
    float   timeSec;
    Vector2 gravity;

    PhysicsSimulation() : deltaTime(0.0f), timeSec(0.0f), gravity{ 0.0f, 800.0f } {}

    void BeginFrame() {
        deltaTime = GetFrameTime();
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        timeSec += deltaTime;
    }

    void Integrate(PhysicsBody& b) const {
        if (!b.active) return;
        b.vel.x += gravity.x * deltaTime;
        b.vel.y += gravity.y * deltaTime;
        b.pos.x += b.vel.x * deltaTime;
        b.pos.y += b.vel.y * deltaTime;
    }
};

//  Globals 

const unsigned int TARGET_FPS = 60;
PhysicsSimulation sim;

float launchX = 200.0f;
float launchY = 500.0f;
float launchAngleDeg = 45.0f;
float launchSpeed = 500.0f;

float gravityMag = 800.0f;
float gravityAngle = 90.0f;

PhysicsBody bird = { {200.0f, 500.0f}, {0.0f, 0.0f}, 0.0f, 1.0f, false };

#define TRAIL_MAX 300
Vector2 trail[TRAIL_MAX];
int trailCount = 0;

static inline void ResetTrail() { trailCount = 0; }
static inline void PushTrail(Vector2 p) { if (trailCount < TRAIL_MAX) trail[trailCount++] = p; }



static inline void UpdateGravityFromUI()
{
    float a = DEG2RAD * gravityAngle;
    sim.gravity.x = gravityMag * cosf(a);
    sim.gravity.y = gravityMag * sinf(a);
}

static inline void Launch()
{
    bird.pos.x = launchX;
    bird.pos.y = launchY;

    float a = DEG2RAD * launchAngleDeg;
    bird.vel.x = launchSpeed * cosf(a);
    bird.vel.y = -launchSpeed * sinf(a);

    bird.active = true;
    ResetTrail();
}

//Update & Draw 

static void update()
{
    sim.BeginFrame();
    UpdateGravityFromUI();

    if (IsKeyPressed(KEY_ONE))   launchAngleDeg = 0.0f;
    if (IsKeyPressed(KEY_TWO))   launchAngleDeg = 45.0f;
    if (IsKeyPressed(KEY_THREE)) launchAngleDeg = 60.0f;
    if (IsKeyPressed(KEY_FOUR))  launchAngleDeg = 90.0f;

    if (IsKeyPressed(KEY_SPACE)) Launch();
    if (IsKeyPressed(KEY_R)) { bird.active = false; ResetTrail(); }

    if (bird.active) {
        sim.Integrate(bird);
        PushTrail(bird.pos);

        if (bird.pos.x < -50 || bird.pos.x > GetScreenWidth() + 50 ||
            bird.pos.y > GetScreenHeight() + 50 || bird.pos.y < -50) {
            bird.active = false;
        }
    }
}

static void draw()
{
    BeginDrawing();
    ClearBackground(BLACK);

    DrawText("Aathiththan Yogeswaran 101462564",
        10, (int)(GetScreenHeight() - 30), 20, LIGHTGRAY);
    DrawText(TextFormat("T: %6.2f", sim.timeSec),
        GetScreenWidth() - 140, 10, 30, LIGHTGRAY);

    // Sliders
    Rectangle rsx = { 10.0f,  15.0f, 320.0f, 20.0f };
    Rectangle rsy = { 10.0f,  40.0f, 320.0f, 20.0f };
    Rectangle rsa = { 10.0f,  65.0f, 320.0f, 20.0f };
    Rectangle rss = { 10.0f,  90.0f, 320.0f, 20.0f };
    Rectangle rgm = { 10.0f, 115.0f, 320.0f, 20.0f };
    Rectangle rga = { 10.0f, 140.0f, 320.0f, 20.0f };

    GuiSliderBar(rsx, "launchPos X", TextFormat("%.0f", launchX),
        &launchX, 50.0f, (float)(GetScreenWidth() - 50));
    GuiSliderBar(rsy, "launchPos Y", TextFormat("%.0f", launchY),
        &launchY, 50.0f, (float)(GetScreenHeight() - 50));
    GuiSliderBar(rsa, "launchAngle", TextFormat("%.1f deg", launchAngleDeg),
        &launchAngleDeg, 0.0f, 180.0f);
    GuiSliderBar(rss, "launchSpeed", TextFormat("%.0f", launchSpeed),
        &launchSpeed, 0.0f, 1400.0f);
    GuiSliderBar(rgm, "gravity mag", TextFormat("%.0f", gravityMag),
        &gravityMag, 0.0f, 2500.0f);
    GuiSliderBar(rga, "gravity angle", TextFormat("%.1f deg", gravityAngle),
        &gravityAngle, 0.0f, 360.0f);

    DrawText("SPACE = launch   R = reset   1/2/3/4 = 0/45/60/90 deg", 10, 165, 18, GRAY);

    // Launch vector preview when idle
    if (!bird.active) {
        float a = DEG2RAD * launchAngleDeg;
        float v0x = launchSpeed * cosf(a);
        float v0y = -launchSpeed * sinf(a);
        float s = 0.35f;

        Vector2 start = { launchX, launchY };
        Vector2 tip = { launchX + v0x * s, launchY + v0y * s };
        DrawCircleV(start, 8.0f, GREEN);
        DrawLineEx(start, tip, 4.0f, RED);
    }

    // Gravity vector visualization
    Vector2 gStart = { 40.0f, 40.0f };
    float s = 0.08f;
    Vector2 gTip = { gStart.x + sim.gravity.x * s, gStart.y + sim.gravity.y * s };
    DrawLineEx(gStart, gTip, 3.0f, YELLOW);
    DrawText("g", 44, 22, 18, YELLOW);

    // Trail + bird
    Color trailCol = { 160, 160, 160, 200 };
    for (int i = 0; i < trailCount; ++i)
        DrawCircleV(trail[i], 2.0f, trailCol);

    if (bird.active) DrawCircleV(bird.pos, 10.0f, RED);

    EndDrawing();
}

//  Main 

int main(void)
{
    InitWindow(InitialWidth, InitialHeight, "GAME2005 Aathiththan Yogeswaran 101462564 - Week 3");
    SetTargetFPS(TARGET_FPS);

    while (!WindowShouldClose())
    {
        update();
        draw();
    }

    CloseWindow();
    return 0;
}
