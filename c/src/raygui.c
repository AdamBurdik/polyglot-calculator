#include "raygui.h"
#include <stdio.h>
#include <string.h>

int calc_calculate(const char *expr, double *out, char *err, size_t errlen);
char buffer[128] = "\0";

static char status[48] = "\0";
static int fresh = 1;
static int last_ok = 0;
static int current_page = 0;

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
    char expr[512];
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
	if (strcmp(label, "2nd") == 0) {
		current_page = 1;
		return;
	}
	if (strcmp(label, "1st") == 0) {
		current_page = 0;
		return;
	}
    if (strcmp(label, "=") == 0) {
        evaluate();
        return;
    }
    if (strcmp(label, "sqrt") == 0) {
        size_t len = strlen(buffer);
        size_t add = strlen(label);
        if (len + add + 2 <= sizeof(buffer)) {
            memcpy(buffer + len, label, add);
            buffer[len + add] = '(';
            buffer[len + add + 1] = '\0';
        }
        fresh = 0;
        return;
    }
    if (
    	strcmp(label, "sin") == 0 ||
    	strcmp(label, "cos") == 0 ||
    	strcmp(label, "tan") == 0
   	) {
        size_t len = strlen(buffer);
        size_t add = strlen(label);
        if (len + add + 2 <= sizeof(buffer)) {
            memcpy(buffer + len, label, add);
            buffer[len + add] = '(';
            buffer[len + add + 1] = '\0';
        }
        fresh = 0;
        return;
    }
    if (
    	strcmp(label, "det2") == 0 ||
    	strcmp(label, "det3") == 0 ||
    	strcmp(label, "tr2") == 0 ||
    	strcmp(label, "tr3") == 0 ||
    	strcmp(label, "frob2") == 0 ||
    	strcmp(label, "frob3") == 0
   	) {
        size_t len = strlen(buffer);
        size_t add = strlen(label);
        if (len + add + 2 <= sizeof(buffer)) {
            memcpy(buffer + len, label, add);
            buffer[len + add] = '(';
            buffer[len + add + 1] = '\0';
        }
        fresh = 0;
        return;
    }
    if (fresh) {
        if ((label[0] >= '0' && label[0] <= '9') || strcmp(label, ".") == 0 || strcmp(label, "(") == 0 ||
            strcmp(label, "pi") == 0 || strcmp(label, "e") == 0) {
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
        if ((key >= '0' && key <= '9') || key == '.' || key == '(' || key == ')' || key == '+' || key == '-' || key == ',') {
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
        if (buttons[i].content[0] == '\0') continue;
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
    const int btnH = 82;
    Color btn = (Color){ 255, 255, 255, 255 };
    Color btnclick = (Color){ 74, 91, 214, 255 };
    Color btnhover = (Color){ 74, 91, 214, 255 };
    Color btnpress = (Color){ 45, 55, 140, 255 };
    
    struct button calcButtons[2][ROWS * COLLUMBS] = {
        {
        	{ btnW,btnH,btn,"(",0,startY + (0 * btnH) },
   	        { btnW,btnH,btn,")",btnW,startY + (0 * btnH) },
   	        { btnW,btnH,btn,"2nd",2 * btnW,startY + (0 * btnH) },
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
   	        { btnW,btnH,btn,"+",3 * btnW,startY + (4 * btnH) },
   	        { btnW,btnH,btn,"pi",0,startY + (5 * btnH) },
   	        { btnW,btnH,btn,"e",btnW,startY + (5 * btnH) },
   	        { btnW,btnH,btn,"^",2 * btnW,startY + (5 * btnH) },
   	        { btnW,btnH,btn,"%",3 * btnW,startY + (5 * btnH) },
   	        { btnW,btnH,btn,"det2",0,startY + (6 * btnH) },
   	        { btnW,btnH,btn,"tr2",btnW,startY + (6 * btnH) },
   	        { btnW,btnH,btn,"frob2",2 * btnW,startY + (6 * btnH) },
   	        { btnW,btnH,btn,",",3 * btnW,startY + (6 * btnH) }
        },
        {
        	{ btnW,btnH,btn,"^",0,startY + (0 * btnH) },
   	        { btnW,btnH,btn,"sqrt",btnW,startY + (0 * btnH) },
   	        { btnW,btnH,btn,"1st",2 * btnW,startY + (0 * btnH) },
   	        { btnW,btnH,btn,"AC",3 * btnW,startY + (0 * btnH) },
   	        { btnW,btnH,btn,"7",0,startY + (1 * btnH) },
   	        { btnW,btnH,btn,"8",btnW,startY + (1 * btnH) },
   	        { btnW,btnH,btn,"9",2 * btnW,startY + (1 * btnH) },
   	        { btnW,btnH,btn,"sin",3 * btnW,startY + (1 * btnH) },
   	        { btnW,btnH,btn,"4",0,startY + (2 * btnH) },
   	        { btnW,btnH,btn,"5",btnW,startY + (2 * btnH) },
   	        { btnW,btnH,btn,"6",2 * btnW,startY + (2 * btnH) },
   	        { btnW,btnH,btn,"cos",3 * btnW,startY + (2 * btnH) },
   	        { btnW,btnH,btn,"1",0,startY + (3 * btnH) },
   	        { btnW,btnH,btn,"2", btnW,startY + (3 * btnH) },
   	        { btnW,btnH,btn,"3",2 * btnW,startY + (3 * btnH) },
   	        { btnW,btnH,btn,"tan",3 * btnW,startY + (3 * btnH) },
   	        { btnW,btnH,btn,"0",0,startY + (4 * btnH) },
   	        { btnW,btnH,btn,".",btnW,startY + (4 * btnH) },
   	        { btnW,btnH,btnclick,"=",2 * btnW,startY + (4 * btnH) },
   	        { btnW,btnH,btn,"%",3 * btnW,startY + (4 * btnH) },
   	        { btnW,btnH,btn,"pi",0,startY + (5 * btnH) },
   	        { btnW,btnH,btn,"e",btnW,startY + (5 * btnH) },
   	        { btnW,btnH,btn,"(",2 * btnW,startY + (5 * btnH) },
   	        { btnW,btnH,btn,")",3 * btnW,startY + (5 * btnH) },
   	        { btnW,btnH,btn,"det3",0,startY + (6 * btnH) },
   	        { btnW,btnH,btn,"tr3",btnW,startY + (6 * btnH) },
   	        { btnW,btnH,btn,"frob3",2 * btnW,startY + (6 * btnH) },
   	        { btnW,btnH,btn,",",3 * btnW,startY + (6 * btnH) }
        }
    };
    InitWindow(sW, sH, "Polyglot Calculator!"); // had to change the name lol
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();
        int i;
        handle_keys();
        handle_mouse(calcButtons[current_page]);
        
        BeginDrawing();
        	ClearBackground(DARKGRAY); 
        	
	        Rectangle displayRect = { 10, 10, (float)sW - 20, (float)startY - 20 };
	        
	        DrawRectangleRounded(displayRect, 0.05f, 4, (Color){ 241, 243, 244, 255 });
	        
	        {
	            int bufFont = 40;
	            int bufWidth = MeasureText(buffer, bufFont);
	            while (bufFont > 14 && bufWidth > sW - 60) {
	                bufFont -= 4;
	                bufWidth = MeasureText(buffer, bufFont);
	            }
	            DrawText(buffer, 30, startY - 70, bufFont, (Color){ 32, 33, 36, 255 });
	        }
	        if (status[0] != '\0') {
	            int statusWidth = MeasureText(status, 32);
	            DrawText(status, sW - 40 - statusWidth, 40, 32, (Color){ 32, 33, 36, 255 });
	        }
	        
            for (i = 0; i < ROWS * COLLUMBS; i++) {
	            int fontSize = 32;
	            int textWidth, textX, textY;
	            Rectangle rect = {
	                (float) (calcButtons[current_page][i].posX + 4),
	                (float) (calcButtons[current_page][i].posY + 4),
	                (float) (calcButtons[current_page][i].width - 8),
	                (float) (calcButtons[current_page][i].height - 8)
            };
            const char* token = calcButtons[current_page][i].content;
            Color baseColor = WHITE;
            Color textColor = (Color){ 60, 64, 67, 255 };
            int drawBorder = 1;
            if (token[0] == '\0') continue;
            if (strcmp(token, "=") == 0) {
                baseColor = (Color){ 66, 133, 244, 255 };
                textColor = WHITE;
                drawBorder = 0;
            } else if (strcmp(token, "+") == 0 || strcmp(token, "-") == 0 || 
                       strcmp(token, "×") == 0 || strcmp(token, "÷") == 0 || 
                       strcmp(token, "AC") == 0 || strcmp(token, "%") == 0 ||
                       strcmp(token, "(") == 0 || strcmp(token, ")") == 0) {
                baseColor = (Color){ 241, 243, 244, 255 };
                drawBorder = 0;
            }
            if (CheckCollisionPointRec(mouse, rect)) {
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                    baseColor = (Color){ (unsigned char)(baseColor.r * 0.9f), (unsigned char)(baseColor.g * 0.9f), (unsigned char)(baseColor.b * 0.9f), 255 };
                } else {
                    baseColor = (Color){ 
                        (unsigned char)(baseColor.r >= 245 ? 255 : baseColor.r + 10), 
                        (unsigned char)(baseColor.g >= 245 ? 255 : baseColor.g + 10), 
                        (unsigned char)(baseColor.b >= 245 ? 255 : baseColor.b + 10), 
                        255 
                    };
                }
            }
            DrawRectangleRounded(rect, 0.08f, 4, baseColor);
            if (drawBorder) {
                DrawRectangleRoundedLines(rect, 0.08f, 4, (Color){ 218, 220, 224, 255 });
            }
            textWidth = MeasureText(token, fontSize);
            textX = (int)rect.x + ((int)rect.width / 2) - (textWidth / 2);
            textY = (int)rect.y + ((int)rect.height / 2) - (fontSize / 2);
	            
            DrawText(token, textX, textY, fontSize, textColor);
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
