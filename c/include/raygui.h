#pragma once
#include "raylib.h"
#define ROWS 7
#define COLLUMBS 4
struct button {
    const int width;
    const int height;
    Color color;
    char content[9];
    const int posX;
    const int posY;
};
int raygui_main(void);
