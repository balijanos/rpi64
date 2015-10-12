#ifndef PI64_H
#define PI64_H

#include "framebuffer.h"

/* ================================ Defaults ================================ */

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 480
#define DEFAULT_BPP 32

static char PI64_VERSION[6]="1.3\0";

static char ROMROOT[5]="rom/\0";
static char ROMNAME[4]="rom\0"; 

typedef enum {INTERPRETER, EDITOR, RUNTIME, GRAPHIC} pi64mode_t;

/* ============================= Data structures ============================ */

struct globalConfig {
    /* Runtime */
    int r,g,b;
    int alpha;
    char filled;
    int fps;
    long long start_ms;
    long long epoch;
    frameBuffer *fb;
    // char *filename;
    lua_State *L;
    int luaerr; /* True if there was an error in the latest iteration. */
    /* Configuration */
    int width;
    int height;
    int border;
    int bpp;
    /* Command line switches */
    int opt_show_fps;
    int opt_full_screen;
	pi64mode_t mode;
	/* ROM name */
	char *romname;
	int resetFlag; 
};

extern struct globalConfig pi64;

int loadProgram(char *);
void resetProgram(void); 

long long mstime(void);

#endif /* PI64_H */
