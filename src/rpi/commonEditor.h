#ifndef COMMONEDITOR_H
#define COMMONEDITOR_H

#include "framebuffer.h"

#define EDITOR_FPS 30

/* Key Held Modifier Bit Masks */
#define CTRL_MASK (1<<0)
#define SHIFT_MASK (1<<1)
#define ALT_MASK (1<<2)
#define META_MASK (1<<3)

#define KEY_REPEAT_PERIOD 2
#define KEY_REPEAT_PERIOD_FAST 1
#define KEY_REPEAT_DELAY 8

typedef struct erow {
    int size;           /* Size of the row, excluding the null term. */
    char *chars;        /* Row content. */
    unsigned char *hl;  /* Syntax highlight type for each character. */
} erow;

typedef struct keyState {
    char translation;
    int counter;
} keyState;

typedef struct syscolor {
    int r,g,b,a;
} syscolor;

/* Syntax highlight types */
#define HL_COLORS 8

#define HL_NORMAL 0
#define HL_ERROR 1
#define HL_COMMENT 2
#define HL_KEYWORD 3
#define HL_STRING 4
#define HL_NUMBER 5
#define HL_FUNCDEF 6
#define HL_LIB 7

#define SYS_COLORS 6

#define RPI_EDITOR 0
#define RPI_SHELL 1
#define RPI_FRAME 2
#define RPI_FONT 3
#define RPI_EDITOR_CURSOR 4
#define RPI_SHELL_CURSOR 5

syscolor sysscheme[SYS_COLORS];

#define RPI_EDITOR_COLOR {0,0,0,255}
#define RPI_SHELL_COLOR {66,66,231,255}
#define RPI_FRAME_COLOR {165,165,255,255}
#define RPI_FONT_COLOR {120,120,120,255}
#define RPI_EDITOR_CURSOR_COLOR {245,245,255,128}
#define RPI_SHELL_CURSOR_COLOR {165,165,255,128}

syscolor hlscheme[HL_COLORS];

#define HL_NORMAL_COLOR {165,165,255,255}
#define HL_ERROR_COLOR {255,0,0,255}
#define HL_COMMENT_COLOR {180,180,0,255}
#define HL_KEYWORD_COLOR {50,255,50,255}
#define HL_STRING_COLOR {0,255,255,255}
#define HL_NUMBER_COLOR {225,100,100,255}
#define HL_FUNCDEF_COLOR {255,255,255,255}
#define HL_LIB_COLOR {200,100,200,255}
/*
 = {
    HL_NORMAL_COLOR,
    HL_ERROR_COLOR,
    HL_COMMENT_COLOR,
    HL_KEYWORD_COLOR,
    HL_STRING_COLOR,
    HL_NUMBER_COLOR,
    HL_FUNCDEF_COLOR,
    HL_LIB_COLOR
};
*/

#define KEY_MAX 512 /* Latest key is excluded */
typedef struct commonEditorConfig {
    int cx,cy;  /* Cursor x and y position in characters */
    unsigned char cblink; /* Show cursor if (cblink & 0x80) == 0 */
    int screenrows; /* Number of rows that we can show */
    int screencols; /* Number of cols that we can show */
    int margin_top, margin_bottom, margin_left, margin_right;
    int rowoff;     /* Row offset on screen */
    int coloff;     /* Column offset on screen */
    int numrows;    /* Number of rows */
    erow *row;      /* Rows */
    time_t lastevent;   /* Last event time, so we can go standby */
    keyState key[KEY_MAX];   /* Remember if a key is pressed / repeated. */
    unsigned int modifiers;  /* Key modifiers held. CTRL & SHIFT & ALT & META  */
    int dirty;      /* File modified but not saved. */
    char *filename; /* Currently open filename */
    frameBuffer *fb;    /* Framebuffer */
    char *err;          /* Error string to display, or NULL if no error. */
    int errline;        /* Error line to highlight if err != NULL. */
   
    char *command;         /* interpreter only */
    
    // v 1.1 quickCommands
    int quickCommand;  /* interpreter only */
    int quickKey;
    char *copyBuffer;
    
} commonEditorConfig;

/* ================================ Prototypes ============================== */

void ceditorDrawCursor(struct commonEditorConfig *CE);
void ceditorDrawPowerOff(struct commonEditorConfig *CE, int x, int y);
void ceditorDrawChars(struct commonEditorConfig *CE);
void ceditorDraw(struct commonEditorConfig *CE);
void csizeEditor(struct commonEditorConfig *CE, frameBuffer *fb);

void ceditorInsertRow(struct commonEditorConfig *CE, int at, char *s);
void ceditorFreeRow(erow *row);
void ceditorDelRow(struct commonEditorConfig *CE, int at);
char *ceditorRowsToString(struct commonEditorConfig *CE, int *buflen);
void ceditorRowInsertChar(struct commonEditorConfig *CE, erow *row, int at, int c);
void ceditorRowAppendString(struct commonEditorConfig *CE, erow *row, char *s);
void ceditorRowDelChar(struct commonEditorConfig *CE, erow *row, int at);
void ceditorInsertChar(struct commonEditorConfig *CE, int c);
void ceditorInsertNewline(struct commonEditorConfig *CE);
void ceditorDelChar(struct commonEditorConfig *CE);
void ceditorDelCharAtCursor(struct commonEditorConfig *CE);
void ceditorUpdateSyntax(struct commonEditorConfig *CE, erow *row);

void ceditorMouseClicked(struct commonEditorConfig *CE, int x, int y, int button);
void ceditorMoveCursor(struct commonEditorConfig *CE, int key);
int pressed_or_repeated(int counter);

// V1.1 gets current line into INTP->command
void ceditorCurrentRowToString(struct commonEditorConfig *CE, char **buffer);
void ceditorDelCharsToCursor(struct commonEditorConfig *CE);
void ceditorDelCharsFromCursor(struct commonEditorConfig *CE);

int BORDER_T;
int BORDER_B;
int BORDER_L;
int BORDER_R;

int POWEROFF_BUTTON_X;
int POWEROFF_BUTTON_Y;
int SAVE_BUTTON_X;
int SAVE_BUTTON_Y;

struct commonEditorConfig *INTP;
struct commonEditorConfig *E;

char *keywords[255]; /* syntax keyword set */

extern void LUA_INTP_Bindings(lua_State *L);

#endif /* COMMONEDITOR_H */ 
