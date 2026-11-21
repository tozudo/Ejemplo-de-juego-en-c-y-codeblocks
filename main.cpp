// Parche definitivo de compatibilidad para MinGW
extern "C" {
    int stat64(const char *path, void *buffer) { return 0; }
    int stat6432(const char *path, void *buffer) { return 0; }
    int stat64i32(const char *path, void *buffer) { return 0; }
    int _stat64(const char *path, void *buffer) { return 0; }
}

#include "raylib.h"
#include <stdio.h>
#include <string.h>

int main() {
    // 1. Inicializamos la ventana redimensionable con el ratón
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    const int ancho = 1000;
    const int alto = 700;
    InitWindow(ancho, alto, "Prueba Lobo 3D - Camara Seguidora y Colisiones");

    // 2. Configuramos la cámara 3D que seguirá al personaje
    Camera3D camera = { 0 };
    Vector3 offsetCamara = { 0.0f, 5.0f, 9.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Posición inicial del lobo.
    // NOTA: Ponemos Y en -0.9f para compensar el centro del modelo de Blender y que pise el suelo.
    Vector3 posicionLobo = { 0.0f, -0.9f, 2.0f };
    float anguloRotacion = 0.0f;
    float escalaLobo = 1.0f;
    float velocidadTraslado = 4.0f;
    float velocidadAnimacion = 1.0f;
    float frameAnimacionCaminar = 0.0f;
    float frameAnimacionCintura = 0.0f;

    // 3. Cargar el modelo correcto y sus animaciones
    Model modeloLobo = LoadModel("loboescalado10.glb");

    int animsCount = 0;
    ModelAnimation *animaciones = LoadModelAnimations("loboescalado10.glb", &animsCount);
    int indiceAnimCintura = (animsCount > 1) ? 1 : 0;

    // 3b. Cargar escenario con arco y su textura PNG
    Vector3 posicionEscenario = { 0.0f, 0.0f, 0.0f };
    float escalaEscenario = 28.0f;
    Model modeloEscenario = LoadModel("escenario arco..obj");
    Texture2D texEscenario = LoadTexture("arcotextura.png");
    for (int i = 0; i < modeloEscenario.materialCount; i++) {
        modeloEscenario.materials[i].maps[MATERIAL_MAP_DIFFUSE].texture = texEscenario;
    }

    // 4. Crear el Suelo de baldosas y la Pared de ladrillos
    Mesh mallaSuelo = GenMeshPlane(20.0f, 20.0f, 1, 1);
    Model modeloSuelo = LoadModelFromMesh(mallaSuelo);
    Image imgSuelo = GenImageChecked(512, 512, 32, 32, LIGHTGRAY, DARKGRAY);
    Texture2D texSuelo = LoadTextureFromImage(imgSuelo);
    UnloadImage(imgSuelo);
    modeloSuelo.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texSuelo;

    Vector3 posicionPared = { 0.0f, 1.0f, -3.0f };
    Vector3 tamanoPared = { 5.0f, 2.0f, 0.6f };
    Mesh mallaPared = GenMeshCube(tamanoPared.x, tamanoPared.y, tamanoPared.z);
    Model modeloPared = LoadModelFromMesh(mallaPared);
    Image imgPared = GenImageChecked(256, 256, 16, 16, RED, MAROON);
    Texture2D texPared = LoadTextureFromImage(imgPared);
    UnloadImage(imgPared);
    modeloPared.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texPared;

    // Cajas de colisión sólidas (paredes y laterales del arco; deja libre el hueco del arco)
    BoundingBox cajasColision[] = {
        // Pared procedural de prueba
        (BoundingBox){
            (Vector3){ posicionPared.x - tamanoPared.x/2.0f, posicionPared.y - tamanoPared.y/2.0f, posicionPared.z - tamanoPared.z/2.0f },
            (Vector3){ posicionPared.x + tamanoPared.x/2.0f, posicionPared.y + tamanoPared.y/2.0f, posicionPared.z + tamanoPared.z/2.0f }
        },
        // Ejemplo de pilar/muro izquierdo del arco (ajusta min y max según tu escenario)
        (BoundingBox){
            (Vector3){ -4.0f, -1.0f, -1.0f },
            (Vector3){ -1.2f,  3.0f,  1.0f }
        },
        // Ejemplo de pilar/muro derecho del arco (entre -1.2 y +1.2 en X queda hueco para pasar)
        (BoundingBox){
            (Vector3){  1.2f, -1.0f, -1.0f },
            (Vector3){  4.0f,  3.0f,  1.0f }
        }
    };
    const int numCajasColision = sizeof(cajasColision) / sizeof(cajasColision[0]);

    SetTargetFPS(60);

    // 5. Bucle principal
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        bool estaMoviendo = false;
        bool reproduciendoCintura = false;

        // Ajustar distancia/altura de seguimiento de la cámara con las flechas
        if (IsKeyDown(KEY_UP))    offsetCamara.y += 4.0f * deltaTime;
        if (IsKeyDown(KEY_DOWN))  offsetCamara.y -= 4.0f * deltaTime;
        if (IsKeyDown(KEY_LEFT))  offsetCamara.z += 6.0f * deltaTime;
        if (IsKeyDown(KEY_RIGHT)) offsetCamara.z -= 6.0f * deltaTime;

        // Guardamos la posición anterior por si colisiona con alguna pared
        Vector3 posicionAntigua = posicionLobo;

        // --- CONTROL DE TECLADO (W-A-S-D y E fluido) ---
        if (IsKeyDown(KEY_E)) {
            reproduciendoCintura = true;
        }
        else {
            if (IsKeyDown(KEY_W)) { posicionLobo.z -= velocidadTraslado * deltaTime; anguloRotacion = 180.0f; estaMoviendo = true; }
            if (IsKeyDown(KEY_S)) { posicionLobo.z += velocidadTraslado * deltaTime; anguloRotacion = -180.0f; estaMoviendo = true; }
            if (IsKeyDown(KEY_A)) { posicionLobo.x -= velocidadTraslado * deltaTime; anguloRotacion = -90.0f;  estaMoviendo = true; }
            if (IsKeyDown(KEY_D)) { posicionLobo.x += velocidadTraslado * deltaTime; anguloRotacion = 90.0f; estaMoviendo = true; }
        }

        // --- SISTEMA DE COLISIONES ---
        // La caja de colisión se adapta a la altura real del personaje asentado en el suelo
        BoundingBox cajaLobo = {
            (Vector3){ posicionLobo.x - 0.4f, posicionLobo.y, posicionLobo.z - 0.4f },
            (Vector3){ posicionLobo.x + 0.4f, posicionLobo.y + 1.8f, posicionLobo.z + 0.4f }
        };

        // Comprueba colisión contra todas las cajas sólidas definidas
        for (int i = 0; i < numCajasColision; i++) {
            if (CheckCollisionBoxes(cajaLobo, cajasColision[i])) {
                posicionLobo = posicionAntigua;
                break;
            }
        }

        // --- ACTUALIZAR CÁMARA PARA QUE SIGA AL LOBO ---
        camera.target = (Vector3){ posicionLobo.x, posicionLobo.y + 1.5f, posicionLobo.z };
        camera.position = (Vector3){
            posicionLobo.x + offsetCamara.x,
            posicionLobo.y + offsetCamara.y,
            posicionLobo.z + offsetCamara.z
        };

        // --- SELECCIÓN Y ACTUALIZACIÓN DE ANIMACIÓN ---
        if (animsCount > 0) {
            if (reproduciendoCintura) {
                frameAnimacionCintura += 24.0f * velocidadAnimacion * deltaTime;
                if (frameAnimacionCintura >= 60.0f) frameAnimacionCintura = 0.0f;
                UpdateModelAnimation(modeloLobo, animaciones[indiceAnimCintura], (int)frameAnimacionCintura);
                frameAnimacionCaminar = 0.0f;
            }
            else if (estaMoviendo) {
                frameAnimacionCaminar += 24.0f * velocidadAnimacion * deltaTime;
                if (frameAnimacionCaminar >= 60.0f) frameAnimacionCaminar = 0.0f;
                UpdateModelAnimation(modeloLobo, animaciones[0], (int)frameAnimacionCaminar);
                frameAnimacionCintura = 0.0f;
            }
            else {
                frameAnimacionCaminar = 0.0f;
                frameAnimacionCintura = 0.0f;
                UpdateModelAnimation(modeloLobo, animaciones[0], 0);
            }
        }

        // --- DIBUJO 3D ---
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
            // Dibujar suelo
            DrawModel(modeloSuelo, (Vector3){ 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);

            // Dibujar escenario importado escalado y texturizado ("escenario arco..obj")
            DrawModelEx(modeloEscenario, posicionEscenario, (Vector3){ 0.0f, 1.0f, 0.0f }, 0.0f,
                        (Vector3){ escalaEscenario, escalaEscenario, escalaEscenario }, WHITE);

            // Dibujar pared de ladrillos
            DrawModel(modeloPared, posicionPared, 1.0f, WHITE);

            // Dibujar el lobo apoyado correctamente en el suelo con la posición acumulada
            DrawModelEx(modeloLobo, posicionLobo, (Vector3){ 0.0f, 1.0f, 0.0f }, anguloRotacion, (Vector3){ escalaLobo, escalaLobo, escalaLobo }, WHITE);

            // Cajas de colisión visibles para ajustarlas
            for (int i = 0; i < numCajasColision; i++) DrawBoundingBox(cajasColision[i], LIME);
        EndMode3D();

        // --- TEXTO INFORMATIVO ---
        DrawText("W-A-S-D: Mover | [E]: Cintura | Flechas: Ajustar camara", 20, 20, 16, DARKGRAY);
        DrawText("El personaje ahora camina apoyado en las baldosas.", 20, 45, 14, RED);

        const char *textoCoords = TextFormat("Posicion X: %.2f | Y: %.2f | Z: %.2f", posicionLobo.x, posicionLobo.y, posicionLobo.z);
        int tamTexto = 24;
        int anchoCaja = MeasureText(textoCoords, tamTexto) + 16;
        DrawRectangle(14, 66, anchoCaja, tamTexto + 8, Fade(RAYWHITE, 0.85f));
        DrawText(textoCoords, 22, 70, tamTexto, BLUE);

        EndDrawing();
    }

    // 6. Liberar memoria
    UnloadTexture(texEscenario);
    UnloadModel(modeloEscenario);
    UnloadTexture(texSuelo);
    UnloadModel(modeloSuelo);
    UnloadTexture(texPared);
    UnloadModel(modeloPared);
    if (animsCount > 0) {
        UnloadModelAnimations(animaciones, animsCount);
    }
    UnloadModel(modeloLobo);

    CloseWindow();
    return 0;
}
