#include "raylib.h"
#include <string.h>
#define ROWS 5
#define COLLUMBS 4
char buffer[20] = "\0";
struct button {
    const int width;
    const int height;
    Color color;
    char content[9]; 
    const int posX;
    const int posY;
};

int main(void) {
    const int sW = 600;
    const int sH = 800;
    const int startY = 224; 
    const int btnW = 150;   
    const int btnH = 115;   
    Color btn = (Color){ 255, 255, 255, 255 }; 
    Color btnclick = (Color){ 74, 91, 214, 255 };   

    struct button calcButtons[ROWS * COLLUMBS] = {
        // row 0
        { btnW,btnH,btn,"(",0,startY + (0 * btnH) },
        { btnW,btnH,btn,")",btnW,startY + (0 * btnH) },
        { btnW,btnH,btn,"%",2 * btnW,startY + (0 * btnH) },
        { btnW,btnH,btn,"AC",3 * btnW,startY + (0 * btnH) },
        // row 1
        { btnW,btnH,btn,"7",0,startY + (1 * btnH) },
        { btnW,btnH,btn,"8",btnW,startY + (1 * btnH) },
        { btnW,btnH,btn,"9",2 * btnW,startY + (1 * btnH) },
        { btnW,btnH,btn,"÷",3 * btnW,startY + (1 * btnH) },
        // row 2
        { btnW,btnH,btn,"4",0,startY + (2 * btnH) },
        { btnW,btnH,btn,"5",btnW,startY + (2 * btnH) },
        { btnW,btnH,btn,"6",2 * btnW,startY + (2 * btnH) },
        { btnW,btnH,btn,"×",3 * btnW,startY + (2 * btnH) },
        // row 3
        { btnW,btnH,btn,"1",0,startY + (3 * btnH) },
        { btnW,btnH,btn,"2", btnW,startY + (3 * btnH) },
        { btnW,btnH,btn,"3",2 * btnW,startY + (3 * btnH) },
        { btnW,btnH,btn,"-",3 * btnW,startY + (3 * btnH) },
        // row 4
        { btnW,btnH,btn,"0",0,startY + (4 * btnH) },
        { btnW,btnH,btn,".",btnW,startY + (4 * btnH) },
        { btnW,btnH,btnclick,"=",2 * btnW,startY + (4 * btnH) }, 
        { btnW,btnH,btn,"+",3 * btnW,startY + (4 * btnH) }
    };
    InitWindow(sW, sH, "weird almagation");
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        if(IsKeyPressed(KEY_ONE)){}

        /////////////////////////////////////////////////////////////////////////////////////// caution tape
        BeginDrawing();
        ClearBackground(DARKGRAY);
         DrawRectangle(10, 10, sW - 20, startY - 20, RAYWHITE);
            DrawText(buffer, 25, startY - 60, 40, BLACK);

                for (int i = 0; i < ROWS * COLLUMBS; i++) {
                    DrawRectangle(
                    calcButtons[i].posX + 1, 
                    calcButtons[i].posY + 1, 
                    calcButtons[i].width - 2, 
                    calcButtons[i].height - 2, 
                    calcButtons[i].color
                );
                int fontSize = 30;
                int textWidth = MeasureText(calcButtons[i].content, fontSize);
                int textX = calcButtons[i].posX + (calcButtons[i].width / 2) - (textWidth / 2);
                int textY = calcButtons[i].posY + (calcButtons[i].height / 2) - (fontSize / 2);
                Color textColor = (calcButtons[i].color.r == 255) ? BLACK : WHITE;
                DrawText(calcButtons[i].content, textX, textY, fontSize, textColor);
            }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}

