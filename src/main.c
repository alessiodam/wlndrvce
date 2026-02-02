#include <stdio.h>
#include <tice.h>
#include <graphx.h>

#define COLOR_BG 255
#define COLOR_FG 0
#define COLOR_ACCENT 191

void drawTopBar();

int main() {
    gfx_Begin();
    gfx_FillScreen(COLOR_BG);
    drawTopBar();

    do {} while (!os_GetCSC());
    gfx_End();
    return 0;
}

void drawTopBar() {
    char topbar[128];
    snprintf(topbar, sizeof(topbar), "WLAN Configuration Utility %s", GIT_SHA);
    gfx_SetColor(COLOR_ACCENT);
    gfx_FillRectangle(0, 0, LCD_WIDTH, 20);
    gfx_SetTextXY(5, 6);
    gfx_PrintString(topbar);
}
