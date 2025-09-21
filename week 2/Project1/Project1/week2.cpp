/*
This project uses the Raylib framework to provide us functionality for math, graphics, GUI, input etc.
See documentation here: https://www.raylib.com/, and examples here: https://www.raylib.com/examples.html
*/

#include "raylib.h"
#include "raymath.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "game.h"

// Window size (define them instead of InitialWidth/Height)
static constexpr int INITIAL_WIDTH = 1000;
static constexpr int INITIAL_HEIGHT = 800;

const unsigned int TARGET_FPS = 50; // frames/second

// Global variables
float dt = 1.0f / TARGET_FPS; // seconds/frame
float timeVal = 0.0f;
float x = 500.0f;
float y = 500.0f;
float frequency = 1.0f;
float amplitude = 100.0f;

float speed = 100.0f;
float angle = 30.0f;

// --- Update world state ---
void update()
{
    dt = 1.0f / TARGET_FPS;

    float frameDt = GetFrameTime();
    if (frameDt > 0.1f) frameDt = 0.1f;
    dt = frameDt;

    timeVal += dt;

    x = x + (-sinf(timeVal * frequency)) * frequency * amplitude * dt;
    y = y + (cosf(timeVal * frequency)) * frequency * amplitude * dt;
}

// --- Draw world state ---
void draw()
{
    BeginDrawing();
    ClearBackground(BLACK);

    DrawText("Joss Moo-Young 123456789", 10, (float)(GetScreenHeight() - 30), 20, LIGHTGRAY);

    // Proper Rectangle initialization (no C compound literals)
    Rectangle slider1{ 10.0f, 15.0f, 1000.0f, 20.0f };
    GuiSliderBar(slider1, "", TextFormat("%.2f", timeVal), &timeVal, 0.0f, 240.0f);

    Rectangle slider2{ 10.0f, 200.0f, 200.0f, 100.0f };
    GuiSliderBar(slider2, "", TextFormat("Speed: %.0f", speed), &speed, -100.0f, 1000.0f);

    Rectangle slider3{ 10.0f, 400.0f, 200.0f, 100.0f };
    GuiSliderBar(slider3, "", TextFormat("Angle: %.0f", angle), &angle, -180.0f, 180.0f);

    DrawText(TextFormat("T: %6.2f", timeVal), GetScreenWidth() - 140, 10, 30, LIGHTGRAY);

    // Example circle (you can toggle back in if needed)
    // DrawCircle((int)x, (int)y, 70, RED);

    DrawCircle(
        (int)(500 + cosf(timeVal * frequency) * amplitude),
        (int)(500 + sinf(timeVal * frequency) * amplitude),
        70.0f, GREEN
    );

    // Line demo
    Vector2 startPos{ 200.0f, (float)(GetScreenHeight() - 200) };
    Vector2 velocity{ cosf(angle) * speed, -sinf(angle) * speed };

    DrawLineEx(startPos, Vector2Add(startPos, velocity), 3.0f, RED);

    EndDrawing();
}

// --- Main entry ---
int main()
{
    InitWindow(INITIAL_WIDTH, INITIAL_HEIGHT, "GAME2005 Joss Moo-Young 123456789");
    SetTargetFPS(TARGET_FPS);

    while (!WindowShouldClose())
    {
        update();
        draw();
    }

    CloseWindow();
    return 0;
}
