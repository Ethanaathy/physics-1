/*
This project uses the Raylib framework to provide us functionality for math, graphics, GUI, input etc.
See documentation here: https://www.raylib.com/, and examples here: https://www.raylib.com/examples.html
*/

#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h" 
#include "game.h"

const unsigned int TARGET_FPS = 50; //frames/second
float dt = 1.0f / TARGET_FPS; //seconds/frame
float time = 0;
float x = 500;
float y = 500;
float frequency = 1;
float amplitude = 100;

//Changes world state
void update()
{
    dt = 1.0f / TARGET_FPS;

    float frameDt = GetFrameTime();
    if (frameDt > 0.1f) frameDt = 0.1f;
    dt = frameDt;

    time += dt;

    x = x + (-sin(time * frequency)) * frequency * amplitude * dt;
    y = y + (cos(time * frequency)) * frequency * amplitude * dt;
}

//Display world state
void draw()
{
    BeginDrawing();
    ClearBackground(BLACK);
    DrawText("Aathiththan Yogeswaran 101462564", 10, (float)(GetScreenHeight() - 30), 20, LIGHTGRAY);

    GuiSliderBar((Rectangle) { 10, 15, 1000, 20 }, "", TextFormat("%.2f", time), & time, 0, 240);
    DrawText(TextFormat("T: %6.2f", time), GetScreenWidth() - 140, 10, 30, LIGHTGRAY);

    DrawCircle((int)x, (int)y, 70, RED);
    DrawCircle(
        (int)(500 + cos(time * frequency) * amplitude),
        (int)(500 + sin(time * frequency) * amplitude),
        70, GREEN
    );

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
