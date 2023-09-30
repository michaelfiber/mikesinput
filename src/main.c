/**
 * A simple project to test out mikesinput.h's compatibility with raylib
 */
#include "raylib.h"
#include "mikesinput.h"

int main(void)
{
    mikesinput_init();

    InitWindow(800, 600, "Mike's Input");

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        mikesinput_poll();

        BeginDrawing();
        ClearBackground(BLACK);
        DrawFPS(10, 10);
        EndDrawing();
    }

    CloseWindow();
}
