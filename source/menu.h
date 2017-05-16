#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <3ds.h>

#define MAX_SELECTED_OPTIONS 0x10

#define COLOR_TITLE 0x0000FF
#define COLOR_NEUTRAL 0xFFFFFF
#define COLOR_SELECTED 0xFF0000
#define COLOR_BACKGROUND 0x000000

#define CONSOLE_ESC(x) "\x1b[" #x
#define CONSOLE_RESET   CONSOLE_ESC(0m)
#define CONSOLE_BLACK   CONSOLE_ESC(30m)
#define CONSOLE_RED	 CONSOLE_ESC(31;1m)
#define CONSOLE_GREEN   CONSOLE_ESC(32;1m)
#define CONSOLE_YELLOW  CONSOLE_ESC(33;1m)
#define CONSOLE_BLUE	CONSOLE_ESC(34;1m)
#define CONSOLE_MAGENTA CONSOLE_ESC(35;1m)
#define CONSOLE_CYAN	CONSOLE_ESC(36;1m)
#define CONSOLE_WHITE   CONSOLE_ESC(37;1m)

#define CONSOLE_REVERSE		CONSOLE_ESC(7m)

typedef struct ConsoleMenu {
	PrintConsole menuConsole;
} ConsoleMenu;

void init_menu(gfxScreen_t screen);
int menu_draw(const char *title, const char *footer, int back, int count, const char *options[]);
void menu_multkey_draw(const char *title, const char* footer, int back, int count, const char *options[], void* data,bool (*callback)(int result, u32 key, void* data));
int *menu_draw_selection(const char *title, int count, const char *options[], const int *preselected);

#ifdef __cplusplus
}
#endif
