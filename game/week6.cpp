// Week 6 – Sphere–Halfspace Overlap (Raylib)
// Aathiththan Yogeswaran 101462564

#include "raylib.h"
#include "raymath.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <vector>
#include <string>

// ----------------------- timing
static const unsigned TARGET_FPS = 50;
static float dt = 1.0f / TARGET_FPS;
static float tAccum = 0.0f;

// ----------------------- base + shapes
enum FizziksShape { CIRCLE, HALF_SPACE };

class FizziksObjekt
{
public:
    bool isStatic = false;
    Vector2 position{ 0,0 };
    Vector2 velocity{ 0,0 };
    float mass = 1.0f;
    std::string name = "objekt";
    Color color = RED;

    virtual ~FizziksObjekt() = default;
    virtual void draw() {}
    virtual FizziksShape Shape() = 0;
};

// ----------------------- circle
class FizziksCircle : public FizziksObjekt
{
public:
    float radius = 20.0f;

    void draw() override
    {
        DrawCircleV(position, radius, color);
        //DrawLineEx(position, position + velocity, 1, color);
    }

    FizziksShape Shape() override { return CIRCLE; }
};

// ----------------------- halfspace (in 2D: infinite line with a normal)
class FizziksHalfspace : public FizziksObjekt
{
private:
    float  rotationDeg = 0.0f;       // visual only
    Vector2 normal{ 0,-1 };           // unit vector

public:
    void setRotationDegrees(float deg)
    {
        rotationDeg = deg;
        normal = Vector2Rotate(Vector2{ 0,-1 }, rotationDeg * DEG2RAD);
        normal = Vector2Normalize(normal);
    }

    float  getRotation() const { return rotationDeg; }
    Vector2 getNormal()  const { return normal; }

    void draw() override
    {
        DrawCircleV(position, 8, color);
        DrawLineEx(position, position + normal * 40.0f, 2.0f, color);

        Vector2 tangent = Vector2Rotate(normal, PI * 0.5f);
        DrawLineEx(position - tangent * 4000.0f, position + tangent * 4000.0f, 1.0f, color);
    }

    FizziksShape Shape() override { return HALF_SPACE; }
};

// ----------------------- overlap tests
static bool CircleCircleOverlap(FizziksCircle* a, FizziksCircle* b)
{
    return Vector2Distance(a->position, b->position) < (a->radius + b->radius);
}

// signed distance: dot((P - X0), n). overlap if dist < radius
static bool CircleHalfspaceOverlap(FizziksCircle* c, FizziksHalfspace* h)
{
    Vector2 disp = Vector2Subtract(c->position, h->position);
    float signedDist = Vector2DotProduct(disp, h->getNormal());
    return signedDist < c->radius;
}

// ----------------------- world
class FizziksWorld
{
private:
    unsigned objektCount = 0;

public:
    std::vector<FizziksObjekt*> objekts;
    Vector2 accelerationGravity{ 0,9 };

    ~FizziksWorld()
    {
        for (auto* p : objekts) delete p;
        objekts.clear();
    }

    void add(FizziksObjekt* o)
    {
        o->name = std::to_string(objektCount++);
        objekts.push_back(o);
    }

    void update()
    {
        for (size_t i = 0; i < objekts.size(); ++i)
        {
            auto* o = objekts[i];
            if (o->isStatic) continue;

            o->position = Vector2Add(o->position, Vector2Scale(o->velocity, dt));
            o->velocity = Vector2Add(o->velocity, Vector2Scale(accelerationGravity, dt));
        }
        checkCollisions();
    }

    void checkCollisions()
    {
        // reset all circles to GREEN first (then paint RED if overlapping)
        for (auto* o : objekts)
            if (o->Shape() == CIRCLE) o->color = GREEN;

        for (int i = 0; i < (int)objekts.size(); ++i)
            for (int j = i + 1; j < (int)objekts.size(); ++j)
            {
                FizziksObjekt* A = objekts[i];
                FizziksObjekt* B = objekts[j];

                FizziksShape aS = A->Shape();
                FizziksShape bS = B->Shape();

                bool hit = false;

                if (aS == CIRCLE && bS == CIRCLE)
                    hit = CircleCircleOverlap((FizziksCircle*)A, (FizziksCircle*)B);
                else if (aS == CIRCLE && bS == HALF_SPACE)
                    hit = CircleHalfspaceOverlap((FizziksCircle*)A, (FizziksHalfspace*)B);
                else if (aS == HALF_SPACE && bS == CIRCLE)
                    hit = CircleHalfspaceOverlap((FizziksCircle*)B, (FizziksHalfspace*)A);

                if (hit)
                {
                    A->color = RED;
                    B->color = RED;
                }
            }
    }
};

// ----------------------- globals for UI
static FizziksWorld world;
static float launchSpeed = 300.0f;     // px/s
static float launchAngleDeg = 45.0f;   // deg
static float halfspaceRotDeg = 25.0f;  // deg

// house-keeping: remove offscreen dynamic circles
static void CleanupOffscreen()
{
    for (int i = 0; i < (int)world.objekts.size(); ++i)
    {
        FizziksObjekt* o = world.objekts[i];
        if (o->Shape() != CIRCLE) continue; // keep halfspaces

        bool off =
            (o->position.y > GetScreenHeight() + 200) || (o->position.y < -200) ||
            (o->position.x > GetScreenWidth() + 200) || (o->position.x < -200);

        if (off)
        {
            delete o;
            world.objekts.erase(world.objekts.begin() + i);
            --i;
        }
    }
}

// ----------------------- frame funcs
static void UpdateFrame()
{
    dt = 1.0f / TARGET_FPS;
    tAccum += dt;

    // rotate halfspace from slider value
    for (auto* o : world.objekts)
        if (o->Shape() == HALF_SPACE)
            ((FizziksHalfspace*)o)->setRotationDegrees(halfspaceRotDeg);

    // spawn circle
    if (IsKeyPressed(KEY_SPACE))
    {
        auto* c = new FizziksCircle();
        c->position = Vector2{ 120.0f, (float)GetScreenHeight() - 120.0f };
        c->velocity = Vector2{
            launchSpeed * cosf(launchAngleDeg * DEG2RAD),
           -launchSpeed * sinf(launchAngleDeg * DEG2RAD)
        };
        c->color = GREEN;
        world.add(c);
    }

    CleanupOffscreen();
    world.update();
}

static void DrawFrame()
{
    BeginDrawing();
    ClearBackground(BLACK);

    DrawText("Aathiththan Yogeswaran 101462564", 12, GetScreenHeight() - 28, 20, LIGHTGRAY);
    DrawText(TextFormat("T: %6.2f", tAccum), GetScreenWidth() - 160, 12, 28, LIGHTGRAY);

    // UI
    GuiSliderBar(Rectangle{ 12, 14, 380, 20 }, "", TextFormat("time %.2f", tAccum), &tAccum, 0.0f, 999.0f);
    GuiSliderBar(Rectangle{ 12, 44, 380, 24 }, "Speed", TextFormat("%.0f px/s", launchSpeed), &launchSpeed, 0.0f, 1200.0f);
    GuiSliderBar(Rectangle{ 12, 74, 380, 24 }, "Angle", TextFormat("%.1f deg", launchAngleDeg), &launchAngleDeg, -5.0f, 90.0f);
    GuiSliderBar(Rectangle{ 12, 104, 380, 24 }, "Gravity Y", TextFormat("%.1f", world.accelerationGravity.y),
        &world.accelerationGravity.y, -50.0f, 50.0f);
    GuiSliderBar(Rectangle{ 12, 134, 380, 24 }, "Halfspace rot", TextFormat("%.1f deg", halfspaceRotDeg),
        &halfspaceRotDeg, -89.0f, 89.0f);

    DrawText("SPACE = spawn circle", 12, 170, 20, GRAY);

    // preview launch vector
    Vector2 start = Vector2{ 120.0f, (float)GetScreenHeight() - 120.0f };
    Vector2 v = Vector2{ launchSpeed * cosf(launchAngleDeg * DEG2RAD),
                             -launchSpeed * sinf(launchAngleDeg * DEG2RAD) };
    DrawLineEx(start, start + v * 0.5f, 3.0f, RED);

    // draw all
    for (auto* o : world.objekts) o->draw();

    EndDrawing();
}

// ----------------------- main
int main()
{
    InitWindow(1280, 720, "GAME2005 – Week 6 Sphere-Halfspace Overlap");
    SetTargetFPS(TARGET_FPS);

    // one halfspace owned by world
    auto* hs = new FizziksHalfspace();
    hs->isStatic = true;
    hs->position = Vector2{ 640, 420 };
    hs->color = GRAY;
    hs->setRotationDegrees(halfspaceRotDeg);
    world.add(hs);

    while (!WindowShouldClose())
    {
        UpdateFrame();
        DrawFrame();
    }

    CloseWindow();
    return 0;
}
