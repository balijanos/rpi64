#ifndef EDITOR_H
#define EDITOR_H

#include "framebuffer.h"

#include "commonEditor.h"

/* ================================ Prototypes ============================== */

/* editor.c */
void initEditor(frameBuffer *fb, int mt, int mb, int ml, int mr);
void sizeEditor(frameBuffer *fb);
int editorOpen(char *filename);
int editorSave(char *filename);
int editorNew(void);
int editorEvents(void);
// void editorSetError(const char *err, int line);
// void editorClearError(void);
int editorFileWasModified(void);
void editorRun(void);
void editorRemoveLines(void);
int getFirstNonSpace(erow *row);

// prototípes for interpreter
#define KEY_REPEAT_PERIOD 2
#define KEY_REPEAT_PERIOD_FAST 1
#define KEY_REPEAT_DELAY 8
int pressed_or_repeated(int);
int is_separator(int);

#endif /* EDITOR_H */
