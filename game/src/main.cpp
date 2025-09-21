/*
This project uses the Raylib framework to provide us functionality for math, graphics, GUI, input etc.
See documentation here: https://www.raylib.com/, and examples here: https://www.raylib.com/examples.html
*/

#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "game.h"

const unsigned int TARGET_FPS = 50;
float dt = 1.0f / TARGET_FPS;
float time = 0;
float x = 500;
float y = 500;
float frequency = 1;
float amplitude = 100;

float speed = 100;
float angle = 30;

Vector2 launchPos = { 200.0f, 600.0f };

void update()
{
    dt = 1.0f / TARGET_FPS;

    float frameDt = GetFrameTime();
    if (frameDt > 0.1f) frameDt = 0.1f;
    dt = frameDt;

    time += dt;

    x = x + (-sinf(time * frequency)) * frequency * amplitude * dt;
    y = y + (cosf(time * frequency)) * frequency * amplitude * dt;
}

void draw()
{
    BeginDrawing();
    ClearBackground(BLACK);
    DrawText("Aathiththan Yogeswaran 101462564", 10, (float)(GetScreenHeight() - 30), 20, LIGHTGRAY);

    GuiSliderBar(Rectangle{ 10, 15, 1000, 20 }, "", TextFormat("%.2f", time), &time, 0.0f, 240.0f);

    GuiSliderBar(Rectangle{ 10, 60, 300, 20 }, "launchPos X", TextFormat("%.0f", launchPos.x), &launchPos.x, 0.0f, (float)GetScreenWidth());
    GuiSliderBar(Rectangle{ 10, 90, 300, 20 }, "launchPos Y", TextFormat("%.0f", launchPos.y), &launchPos.y, 0.0f, (float)GetScreenHeight());

    GuiSliderBar(Rectangle{ 10, 200, 200, 100 }, "", TextFormat("Speed: %.0f", speed), &speed, -100.0f, 1000.0f);
    GuiSliderBar(Rectangle{ 10, 400, 200, 100 }, "", TextFormat("Angle: %.0f", angle), &angle, -180.0f, 180.0f);

    DrawText(TextFormat("T: %6.2f", time), GetScreenWidth() - 140, 10, 30, LIGHTGRAY);
    DrawText(TextFormat("Pos: (%.0f, %.0f)", launchPos.x, launchPos.y), 10, 190, 20, LIGHTGRAY);
    DrawText(TextFormat("Speed: %.0f", speed), 10, 215, 20, LIGHTGRAY);
    DrawText(TextFormat("Angle: %.0f deg", angle), 10, 240, 20, LIGHTGRAY);

    DrawCircle((int)(500 + cosf(time * frequency) * amplitude),
        (int)(500 + sinf(time * frequency) * amplitude),
        70.0f, GREEN);

    float angRad = angle * DEG2RAD;
    Vector2 v0 = { cosf(angRad) * speed, -sinf(angRad) * speed };
    DrawLineEx(launchPos, Vector2Add(launchPos, v0), 3.0f, RED);
    DrawCircleV(launchPos, 6.0f, MAROON);

    EndDrawing();
}

int main()
{
    InitWindow(InitialWidth, InitialHeight, "GAME2005 Aathiththan Yogeswaran 101462564");
    SetTargetFPS(TARGET_FPS);

    while (!WindowShouldClose())
    {
        update();
        draw();
    }

    CloseWindow();
    return 0;
}
