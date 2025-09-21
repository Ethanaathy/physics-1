#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "game.h"

const unsigned int TARGET_FPS = 50;
float dt = 1.0f / TARGET_FPS;
float timeSec = 0.0f;

float x = 500.0f;
float y = 500.0f;
float frequency = 1.0f;   // a
float amplitude = 100.0f; // b

void update()
{
    dt = GetFrameTime();
    if (dt > 0.1f) dt = 0.1f;
    timeSec += dt;

    x += (-sinf(timeSec * frequency)) * frequency * amplitude * dt;
    y += (cosf(timeSec * frequency)) * frequency * amplitude * dt;
}

void draw()
{
    BeginDrawing();
    ClearBackground(BLACK);

    DrawText("Aathiththan Yogeswaran 101462564",
        10, (int)(GetScreenHeight() - 30), 20, LIGHTGRAY);

    Rectangle rSlider = { 10.0f, 15.0f, 1000.0f, 20.0f };
    GuiSliderBar(rSlider, "", TextFormat("%.2f", timeSec), &timeSec, 0.0f, 240.0f);

    DrawText(TextFormat("T: %6.2f", timeSec),
        GetScreenWidth() - 140, 10, 30, LIGHTGRAY);

    DrawCircle((int)x, (int)y, 70, RED);

    int gx = (int)(500.0f + cosf(timeSec * frequency) * amplitude);
    int gy = (int)(500.0f + sinf(timeSec * frequency) * amplitude);
    DrawCircle(gx, gy, 70, GREEN);

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
