#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "framebuffer.h"
#include "commonEditor.h"

/* ================================ Prototypes ============================== */

/* interpreter.c */
void initInterpreter(frameBuffer *fb, int mt, int mb, int ml, int mr);
char *interpreterRowsToString(int *buflen);
int interpreterEvents(void);
void interpreterRun(void);
void interpreterDraw(void);

int interpreterIsCommand(void);
void interpreterSetCommand(char *);
char *interpreterGetCommand(void);
void interpreterAppendRow(char *);
void interpreterUpdateRow(char *);
void clearKeywords(void);
void rom_interpret(char *);
int execCommand(char *cmd);
void interpreterSetKeyword(char *keyword);
int dirExists(char *);
int fileExists(char *);

// v1.1
void rom_quickcmd(char *cmd);
int interpreterIsQuickCommand(void);
// -----    

extern void LUA_INTP_Bindings(lua_State *L);

#endif /* INTERPRETER_H */
