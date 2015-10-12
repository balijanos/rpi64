#include "commonEditor.h"
#include "editor.h"
#include "pi64.h"

#ifdef WINDOWS
       #include "usleep.h"
#endif

#include <time.h>
#include <unistd.h>


/* ========================= Editor events handling  ======================== */

int editorEvents(void) {
    SDL_Event event;
    int j, ksym;
    time_t idletime;
    int isCtrl;

    /* Sleep 0.25 seconds if no body is pressing any key for a few seconds.
     * This way we can save tons of energy when in editing mode and
     * the user is thinking or away from keyboard. */
    idletime = time(NULL)-E->lastevent;
    if (idletime > 5) {
        #ifdef _WIN32
        usleep((idletime < 60) ? 50 : 250);
        #else
        usleep((idletime < 60) ? 50000 : 250000);
        #endif
        E->cblink = 0;
    }

    while (SDL_PollEvent(&event)) {
        E->lastevent = time(NULL);
        switch(event.type) {
        /* Key pressed */
        case SDL_KEYDOWN:
            ksym = event.key.keysym.sym;
            switch(ksym) {
            case SDLK_ESCAPE:                  
                 return 1;                   
                 break;
            default:
                if (ksym >= 0 && ksym < KEY_MAX) {
                    E->key[ksym].counter = 1;
                    E->key[ksym].translation = (event.key.keysym.unicode & 0xff);
                }
                switch(ksym) {
                case SDLK_LSHIFT:
                case SDLK_RSHIFT:
                    E->modifiers |= SHIFT_MASK;
                    break;
                case SDLK_LCTRL:
                case SDLK_RCTRL:
                    E->modifiers |= CTRL_MASK;
                    break;
                case SDLK_LALT:
                case SDLK_RALT:
                    E->modifiers |= ALT_MASK;
                    break;
                case SDLK_LMETA:
                case SDLK_RMETA:
                    E->modifiers |= META_MASK;
                    break;
                }
                break;
            }
            break;

        /* Key released */
        case SDL_KEYUP:
            ksym = event.key.keysym.sym;
            if (ksym >= 0 && ksym < KEY_MAX) E->key[ksym].counter = 0;
            switch(ksym) {
            case SDLK_LSHIFT:
            case SDLK_RSHIFT:
                E->modifiers &= ~SHIFT_MASK;
                break;
            case SDLK_LCTRL:
            case SDLK_RCTRL:
                E->modifiers &= ~CTRL_MASK;
                break;
            case SDLK_LALT:
            case SDLK_RALT:
                E->modifiers &= ~ALT_MASK;
                break;
            case SDLK_LMETA:
            case SDLK_RMETA:
                E->modifiers &= ~META_MASK;
                break;
            }
            break;
        /* Mouse click */
        case SDL_MOUSEBUTTONDOWN:
            ceditorMouseClicked(E,event.motion.x, E->fb->height-1-event.motion.y,event.button.button);
            if (pi64.resetFlag==1)
               return 1;
            break;
        case SDL_QUIT:
            exit(0);
            break;
        }
    }
    
    isCtrl=0;
    /* Convert events into actions */
    for (j = 0; j < KEY_MAX; j++) {
        int i;

        if (pressed_or_repeated(E->key[j].counter)) {
            E->lastevent = time(NULL);
            E->cblink = 0;
            switch(j) {
            case SDLK_LEFT:
            case SDLK_RIGHT:
            case SDLK_UP:
            case SDLK_DOWN:
            case SDLK_PAGEUP:
            case SDLK_PAGEDOWN:
            case SDLK_HOME:
            case SDLK_END:
                ceditorMoveCursor(E,j);
                break;
            case SDLK_BACKSPACE:
                ceditorDelChar(E);
                break;
            case SDLK_DELETE:
                ceditorDelCharAtCursor(E);
                break;
            case SDLK_RETURN:
                ceditorInsertNewline(E);
                break;
            case SDLK_LSHIFT:
            case SDLK_RSHIFT:
                 break;
            case SDLK_LCTRL:
            case SDLK_RCTRL:
                 isCtrl=1;
                 break;
            case SDLK_LALT:
            case SDLK_RALT:
            case SDLK_LMETA:
            case SDLK_RMETA:
            case SDLK_NUMLOCK:
	        case SDLK_CAPSLOCK:
	        case SDLK_SCROLLOCK:
                /* Ignored */
                break;
            case SDLK_TAB:
                for (i = 0; i < 4; i++)
                    ceditorInsertChar(E,' ');
                break;
                
            default:                
                if (E->modifiers & CTRL_MASK && !(E->modifiers & ALT_MASK) ) {  
                   E->key[j].counter--;                
                   switch(j) {
                   // CTRL+B
                        case 98:                   
                             ceditorDelCharsToCursor(E);
                             break;
                   // CTRL+C
                        case 99:                   
                             ceditorCurrentRowToString(E, &E->copyBuffer);
                             break;
                   // CTRL+D
                        case 100:
                             ceditorDelRow(E, -1);
                             // if (E->cy > 0) E->cy--;
                             E->cx = 0;
                             break;
                   // CTRL+E
                        case 101:                   
                             ceditorDelCharsFromCursor(E);
                             break;
                   // CTRL+I
                        case 105:                   
                             ceditorInsertRow(E,-1,"");
                             E->cy++;
                             E->cx = 0;
                             break;
                   // CTRL+S
                        case 115:                             
                             editorSave(E->filename);
                             break;
                   // CTRL+V
                        case 118:
                             if (E->copyBuffer != NULL) {             
                                ceditorInsertRow(E, -1, E->copyBuffer);
                                 E->cx = 0;
                                 E->cy++;
                             }
                             break;
                   // CTRL+X
                        case 120:
                             ceditorCurrentRowToString(E, &E->copyBuffer);
                             ceditorDelRow(E, -1);
                             E->cx = 0;
                             // if (E->cy > 0) E->cy--;
                             break;
                             
                        default:                              
                             ceditorInsertChar(E, E->key[j].translation);
                             break;
                   }
                } else {
                   ceditorInsertChar(E,E->key[j].translation);
                }
                break;
            }
        }
        if (E->key[j].counter) E->key[j].counter++; /* auto repeat counter */
    }

    /* Call the draw function at every iteration.  */
    ceditorDraw(E);
    /* Refresh the screen */
    SDL_Flip(E->fb->screen);
    SDL_framerateDelay(&E->fb->fps_mgr);
    return 0;
}

/* Load the specified program in the editor memory and returns 0 on success
 * or 1 on error. */
int editorOpen(char *filename) {
    FILE *fp;
    char line[1024];
    
    int mode = pi64.mode;
    pi64.mode = EDITOR;
    
    editorRemoveLines();
                
    fp = fopen(filename,"r");
    if (!fp) {
        perror("fopen loading program into editor");
        pi64.mode = mode;
        return 1;
    }
    while(fgets(line,sizeof(line),fp) != NULL) {
        int l = strlen(line);

        if (l && (line[l-1] == '\n' || line[l-1] == '\r'))
            line[l-1] = '\0';
        ceditorInsertRow(E, E->numrows,line);
    }
    fclose(fp);
    E->dirty = 0;
    E->cx = 0;
    E->cy = 0;
    E->cblink = 0;
    E->rowoff = 0;
    E->coloff = 0;
    free(E->filename);
    E->filename = strdup(filename);
    pi64.mode = mode;
    return 0;
}

int editorSave(char *filename) {
    int len;
    char *buf = ceditorRowsToString(E, &len);
    FILE *fp;

    fp = fopen(filename,"w");
    if (!fp) {
        free(buf);
        return 1;
    }
    fwrite(buf,len,1,fp);
    fclose(fp);
    free(buf);
    E->dirty = 0;
    E->filename=strdup(filename);
    return 0;
}


int editorFileWasModified(void) {
    return E->dirty;
}

void editorRun(void) {
    int j;
    for (j = 0; j < KEY_MAX; j++) {
        E->key[j].counter = 0;
    }
    E->lastevent = time(NULL);
    SDL_setFramerate(&E->fb->fps_mgr,EDITOR_FPS);
    while(!editorEvents());
}

void editorRemoveLines(void) {
    // removw previous runs garbage 
    int i;
    for (i=E->numrows-1; 0; i--) {
        erow *row = &E->row[i];
        if (row) {
           free(row->chars);
           free(row->hl);
        }
    } 
    E->numrows = 0;
}

int editorNew(void) {
    editorRemoveLines();
    if (E->filename!=NULL)
       free(E->filename);
    E->filename = NULL;
    E->cx = 0;
    E->cy = 0;
    E->cblink = 0;
    E->rowoff = 0;
    E->coloff = 0;
    return 0;
}


void initEditor(frameBuffer *fb, int mt, int mb, int ml, int mr) {
    editorRemoveLines();
    E->fb = fb;
    E->cx = 0;
    E->cy = 0;
    E->cblink = 0;
    E->rowoff = 0;
    E->coloff = 0;
    E->numrows = 0;
    E->row = NULL;
    E->margin_top = mt;
    E->margin_bottom = mb;
    E->margin_left = ml;
    E->margin_right = mr;
    E->screencols = (fb->width-E->margin_left-E->margin_right) / fontKerning;
    E->screenrows = (fb->height-E->margin_top-E->margin_bottom) / fontHeight;
    E->dirty = 0;
    E->filename = NULL;
    memset(E->key,0,sizeof(E->key));
    E->modifiers = 0;
    E->quickCommand = 0; 
    E->quickKey = 0;
    E->copyBuffer = NULL;
}

