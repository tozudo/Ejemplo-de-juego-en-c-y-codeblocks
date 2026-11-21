#include "raylib.h"

int main()
{
    // Inicializamos la ventana gráfica
    const int ancho = 800;
    const int alto = 600;
    InitWindow(ancho, alto, "Mi primer juego en C++ con Raylib");

    SetTargetFPS(60);

    // Bucle principal del juego
    while (!WindowShouldClose())
    {
        BeginDrawing();
            ClearBackground(RAYWHITE);

            DrawText("¡Proyecto C++ funcionando a la primera!", 150, 250, 20, DARKGRAY);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
