#include "raygui.h"
#include <stdio.h>
#include <string.h>
int calc_calculate(const char *expr, double *out, char *err, size_t errlen);
char buffer[20] = "\0";
static char status[48] = "\0";
static int fresh = 1;
static int last_ok = 0;
static void build_expr(const char *src, char *dst, size_t cap) {
    size_t j = 0;
    while (*src != '\0' && j + 1 < cap) {
        if (strncmp(src, "÷", 2) == 0) {
            dst[j++] = '/';
            src += 2;
        } else if (strncmp(src, "×", 2) == 0) {
            dst[j++] = '*';
            src += 2;
        } else if (*src == '%') {
            if (j + 5 <= cap) {
                memcpy(dst + j, "/100", 4);
                j += 4;
            }
            src++;
        } else {
            dst[j++] = *src++;
        }
    }
    dst[j] = '\0';
}

static void evaluate(void) {
    char expr[128];
    char err[256];
    double out = 0.0;
    build_expr(buffer, expr, sizeof(expr));
    err[0] = '\0';
    if (calc_calculate(expr, &out, err, sizeof(err)) != 0) {
        snprintf(status, sizeof(status), "error: %.34s", err[0] != '\0' ? err : "failed");
        last_ok = 0;
    } else {
        snprintf(status, sizeof(status), "%.12g", out);
        last_ok = 1;
    }
    fresh = 1;
}

static void press(const char *label) {
    size_t len;
    size_t add;
    if (strcmp(label, "AC") == 0) {
        buffer[0] = '\0';
        status[0] = '\0';
        fresh = 1;
        last_ok = 0;
        return;
    }
    if (strcmp(label, "=") == 0) {
        evaluate();
        return;
    }
    if (fresh) {
        if ((label[0] >= '0' && label[0] <= '9') || strcmp(label, ".") == 0 || strcmp(label, "(") == 0) {
            buffer[0] = '\0';
        } else if (last_ok && status[0] != '\0') {
            snprintf(buffer, sizeof(buffer), "%s", status);
        } else {
            buffer[0] = '\0';
        }
        fresh = 0;
    }
    len = strlen(buffer);
    add = strlen(label);
    if (len + add + 1 <= sizeof(buffer)) {
        memcpy(buffer + len, label, add + 1);
    }
}

static void backspace(void) {
    size_t len = strlen(buffer);
    if (len == 0) return;
    len--;
    while (len > 0 && ((unsigned char) buffer[len] & 0xC0) == 0x80) len--;
    buffer[len] = '\0';
    fresh = 0;
}

static void handle_keys(void) {
    int key;
    char text[2];
    text[1] = '\0';
    while ((key = GetCharPressed()) > 0) {
        if (key > 126 || key < 32) continue;
        text[0] = (char) key;
        if ((key >= '0' && key <= '9') || key == '.' || key == '(' || key == ')' || key == '+' || key == '-') {
            press(text);
        } else if (key == '*') {
            press("×");
        } else if (key == '/') {
            press("÷");
        } else if (key == '%') {
            press("%");
        } else if (key == '=') {
            press("=");
        }
    }
    if (IsKeyPressed(KEY_BACKSPACE)) backspace();
    if (IsKeyPressed(KEY_DELETE)) press("AC");
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) press("=");
}

static void handle_mouse(const struct button *buttons) {
    Vector2 mouse;
    int i;
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return;
    mouse = GetMousePosition();
    for (i = 0; i < ROWS * COLLUMBS; i++) {
        Rectangle rect = {
            (float) (buttons[i].posX + 1),
            (float) (buttons[i].posY + 1),
            (float) (buttons[i].width - 2),
            (float) (buttons[i].height - 2)
        };
        if (CheckCollisionPointRec(mouse, rect)) {
            press(buttons[i].content);
            return;
        }
    }
}

int raygui_main(void) {
    const int sW = 600;
    const int sH = 800;
    const int startY = 224;
    const int btnW = 150;
    const int btnH = 115;
    Color btn = (Color){ 255, 255, 255, 255 };
    Color btnclick = (Color){ 74, 91, 214, 255 };
    Color btnhover = (Color){ 74, 91, 214, 255 };
    Color btnpress = (Color){ 45, 55, 140, 255 };
    struct button calcButtons[ROWS * COLLUMBS] = {
        { btnW,btnH,btn,"(",0,startY + (0 * btnH) },
        { btnW,btnH,btn,")",btnW,startY + (0 * btnH) },
        { btnW,btnH,btn,"%",2 * btnW,startY + (0 * btnH) },
        { btnW,btnH,btn,"AC",3 * btnW,startY + (0 * btnH) },
        { btnW,btnH,btn,"7",0,startY + (1 * btnH) },
        { btnW,btnH,btn,"8",btnW,startY + (1 * btnH) },
        { btnW,btnH,btn,"9",2 * btnW,startY + (1 * btnH) },
        { btnW,btnH,btn,"÷",3 * btnW,startY + (1 * btnH) },
        { btnW,btnH,btn,"4",0,startY + (2 * btnH) },
        { btnW,btnH,btn,"5",btnW,startY + (2 * btnH) },
        { btnW,btnH,btn,"6",2 * btnW,startY + (2 * btnH) },
        { btnW,btnH,btn,"×",3 * btnW,startY + (2 * btnH) },
        { btnW,btnH,btn,"1",0,startY + (3 * btnH) },
        { btnW,btnH,btn,"2", btnW,startY + (3 * btnH) },
        { btnW,btnH,btn,"3",2 * btnW,startY + (3 * btnH) },
        { btnW,btnH,btn,"-",3 * btnW,startY + (3 * btnH) },
        { btnW,btnH,btn,"0",0,startY + (4 * btnH) },
        { btnW,btnH,btn,".",btnW,startY + (4 * btnH) },
        { btnW,btnH,btnclick,"=",2 * btnW,startY + (4 * btnH) },
        { btnW,btnH,btn,"+",3 * btnW,startY + (4 * btnH) }
    };
    InitWindow(sW, sH, "weird almagation");
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();
        int i;
        handle_keys();
        handle_mouse(calcButtons);
        BeginDrawing();
        ClearBackground(DARKGRAY);
        DrawRectangle(10, 10, sW - 20, startY - 20, RAYWHITE);
        DrawText(buffer, 25, startY - 60, 40, BLACK);
        if (status[0] != '\0') {
            int statusWidth = MeasureText(status, 24);
            DrawText(status, sW - 25 - statusWidth, 30, 24, GRAY);
        }

        for (i = 0; i < ROWS * COLLUMBS; i++) {
            Rectangle rect = {
                (float) (calcButtons[i].posX + 1),
                (float) (calcButtons[i].posY + 1),
                (float) (calcButtons[i].width - 2),
                (float) (calcButtons[i].height - 2)
            };
            Color color = calcButtons[i].color;
            int fontSize = 30;
            int textWidth;
            int textX;
            int textY;
            Color textColor;
            if (CheckCollisionPointRec(mouse, rect)) {
                color = IsMouseButtonDown(MOUSE_BUTTON_LEFT) ? btnpress : btnhover;
            }
            DrawRectangle((int) rect.x, (int) rect.y, (int) rect.width, (int) rect.height, color);
            textWidth = MeasureText(calcButtons[i].content, fontSize);
            textX = calcButtons[i].posX + (calcButtons[i].width / 2) - (textWidth / 2);
            textY = calcButtons[i].posY + (calcButtons[i].height / 2) - (fontSize / 2);
            textColor = (color.r > 200) ? BLACK : WHITE;
            DrawText(calcButtons[i].content, textX, textY, fontSize, textColor);
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
