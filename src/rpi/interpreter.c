#include <time.h>
#include <unistd.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "commonEditor.h"
#include "interpreter.h"
#include "pi64.h"

#ifdef WINDOWS
       #include "usleep.h"
#endif

/* ====================== Syntax highlight color scheme  ==================== */

void interpreterAppendRow(char *s) {
    int filerow = INTP->rowoff+INTP->cy;
    ceditorDelRow(INTP,filerow);
    ceditorInsertRow(INTP,filerow,s);
    if (INTP->cy == INTP->screenrows-1) {
        INTP->rowoff++;
    } else {
        INTP->cy++;
    }
}

void interpreterUpdateRow(char *s) {
    int filerow = INTP->rowoff+INTP->cy;
    ceditorDelRow(INTP,filerow);
    ceditorInsertRow(INTP,filerow,s);    
}

/* ========================= Interpreter events handling  ======================== */
int interpreterIsCommand(void) {    
    return (INTP->command!=NULL);
}

int interpreterIsQuickCommand(void) {    
    return (INTP->quickCommand!=0);
}

char *interpreterGetCommand(void) {
    char *cmd;
    if (INTP->command == NULL) return NULL;
    cmd = strdup(INTP->command);
    free(INTP->command);
    INTP->command = NULL;
    return cmd;
}

int interpreterGetQuickCommand(void) {
    int qcmd;    
    qcmd = INTP->quickCommand;
    INTP->quickCommand = 0;
    return qcmd;
}

int interpreterEvents(void) {
    SDL_Event event;
    int j, ksym;
    time_t idletime;

    /* Sleep 0.25 seconds if no body is pressing any key for a few seconds.
     * This way we can save tons of energy when in editing mode and
     * the user is thinking or away from keyboard. */
    idletime = time(NULL)-INTP->lastevent;
    if (idletime > 5) {
        #ifdef _WIN32
        usleep((idletime < 60) ? 50 : 250);
        #else
        usleep((idletime < 60) ? 50000 : 250000);
        #endif
        INTP->cblink = 0;
    }

    while (SDL_PollEvent(&event)) {
        INTP->lastevent = time(NULL);
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
                    INTP->key[ksym].counter = 1;
                    INTP->key[ksym].translation = (event.key.keysym.unicode & 0xff);
                }                
                switch(ksym) {
                case SDLK_LSHIFT:
                case SDLK_RSHIFT:
                    INTP->modifiers |= SHIFT_MASK;
                    break;
                case SDLK_LCTRL:
                case SDLK_RCTRL:
                    INTP->modifiers |= CTRL_MASK;
                    break;
                case SDLK_LALT:
                     INTP->quickKey = 1;
                     INTP->modifiers |= ALT_MASK;
                     break;                
                case SDLK_RALT:
                    INTP->modifiers |= ALT_MASK;
                    break;
                case SDLK_LMETA:
                case SDLK_RMETA:
                    INTP->modifiers |= META_MASK;
                    break;
                }
                break;
            }
            break;

        /* Key released */
        case SDL_KEYUP:
            ksym = event.key.keysym.sym;
            if (ksym >= 0 && ksym < KEY_MAX) INTP->key[ksym].counter = 0;
            switch(ksym) {
            case SDLK_LSHIFT:
            case SDLK_RSHIFT:
                INTP->modifiers &= ~SHIFT_MASK;
                break;
            case SDLK_LCTRL:
            case SDLK_RCTRL:
                INTP->modifiers &= ~CTRL_MASK;
                break;
            case SDLK_LALT:
                 INTP->quickKey = 0;
                 INTP->modifiers &= ~ALT_MASK;
                 break;    
            case SDLK_RALT:
                INTP->modifiers &= ~ALT_MASK;
                break;
            case SDLK_LMETA:
            case SDLK_RMETA:
                INTP->modifiers &= ~META_MASK;
                break;
            }
            break;
        /* Mouse click */
        case SDL_MOUSEBUTTONDOWN:
            ceditorMouseClicked(INTP, event.motion.x, INTP->fb->height-1-event.motion.y,event.button.button);
            if (pi64.resetFlag==1)
               return 1;
            break;
        case SDL_QUIT:
            exit(0);
            break;
        }
    }

    /* Convert events into actions */
    for (j = 0; j < KEY_MAX; j++) {
        int i;        
        if (pressed_or_repeated(INTP->key[j].counter)) {                     
            INTP->lastevent = time(NULL);
            INTP->cblink = 0;
            switch(j) {
            case SDLK_LEFT:
            case SDLK_RIGHT:
            case SDLK_UP:
            case SDLK_DOWN:
            case SDLK_PAGEUP:
            case SDLK_PAGEDOWN:
            case SDLK_HOME:
            case SDLK_END:
                ceditorMoveCursor(INTP, j);
                break;
            case SDLK_BACKSPACE:
                ceditorDelChar(INTP);
                break;
            case SDLK_DELETE:
                ceditorDelCharAtCursor(INTP);
                break;
            case SDLK_RETURN:
                ceditorInsertNewline(INTP);
                break;
            
            /* Ignored */          
            case SDLK_LSHIFT:
            case SDLK_RSHIFT:
            case SDLK_LCTRL:
            case SDLK_RCTRL:
            case SDLK_LALT: 
            case SDLK_RALT:
            case SDLK_LMETA:
            case SDLK_RMETA:
            case SDLK_NUMLOCK:
	        case SDLK_CAPSLOCK:
	        case SDLK_SCROLLOCK:              
                break;            
            
            case SDLK_TAB:
                for (i = 0; i < 4; i++)
                    ceditorInsertChar(INTP, ' ');
                break;
                
            default:     
                // v1.1 quickcommand 
                if (INTP->quickKey) {                                     
                     INTP->quickCommand = (int) INTP->key[j].translation;  
                     INTP->quickKey = 0;
                     INTP->key[j].counter = 0;
                } else
                if (INTP->modifiers & CTRL_MASK) { 
                   INTP->key[j].counter--;                   
                   switch(j) {
                   // CTRL+B
                        case 98:                   
                             ceditorDelCharsToCursor(INTP);
                             break;                             
                   // CTRL+D
                        case 100:
                             ceditorDelRow(INTP, -1);
                             // if (INTP->cy > 0) INTP->cy--;
                             INTP->cx = 0;
                             break;
                   // CTRL+E
                        case 101:                   
                             ceditorDelCharsFromCursor(INTP);
                             break;      
                   // CTRL+I
                        case 105:                   
                             ceditorInsertRow(INTP,-1,"");
                             INTP->cy++;
                             INTP->cx = 0;
                             break;
                                                                                
                        default:                              
                             ceditorInsertChar(INTP, INTP->key[j].translation);
                             break;
                   } 
                   // INTP->key[j].counter = 0;
                } else {
                  // ----------------------------            
                  ceditorInsertChar(INTP, INTP->key[j].translation);
                }

                break;
            }
        }
        if (INTP->key[j].counter) INTP->key[j].counter++; /* auto repeat counter */
    }

    /* Call the draw function at every iteration.  */
    ceditorDraw(INTP); 
    /* Refresh the screen */
    SDL_Flip(INTP->fb->screen);
    SDL_framerateDelay(&INTP->fb->fps_mgr);
    // return event if command!!!!
    return ( interpreterIsCommand() || interpreterIsQuickCommand() );
    // return 0;
}

void interpreterRun(void) {
    int j;
    for (j = 0; j < KEY_MAX; j++) {
        INTP->key[j].counter = 0;
    } 
    INTP->lastevent = time(NULL);
    // SDL_setFramerate(&INTP->fb->fps_mgr,INTERPRETER_FPS);
    while(!interpreterEvents());
}

void interpreterDraw(void) {
    ceditorDraw(INTP);
    /* Refresh the screen */
    SDL_Flip(INTP->fb->screen);
    SDL_framerateDelay(&INTP->fb->fps_mgr);
}

void initInterpreter(frameBuffer *fb, int mt, int mb, int ml, int mr) {     
    // removw previous runs garbage 
    int i;
    for (i=INTP->numrows-1; 0; i--) {
        erow *row = &INTP->row[i];
        if (row) {
           free(row->chars);
           free(row->hl);
        }
    }

    //    
    INTP->fb = fb;
    INTP->cx = 0;
    INTP->cy = 0;
    INTP->cblink = 0;
    INTP->rowoff = 0;
    INTP->coloff = 0;
    INTP->numrows = 0;
    INTP->row = NULL;
    INTP->margin_top = mt;
    INTP->margin_bottom = mb;
    INTP->margin_left = ml;
    INTP->margin_right = mr;
    INTP->screencols = (fb->width-INTP->margin_left-INTP->margin_right) / fontKerning;
    INTP->screenrows = (fb->height-INTP->margin_top-INTP->margin_bottom) / fontHeight;
    INTP->dirty = 0;
    INTP->filename = NULL;
    INTP->command = NULL;
    INTP->quickCommand = 0;

    memset(INTP->key,0,sizeof(INTP->key));
    INTP->modifiers = 0;   
    
    INTP->quickCommand = 0; 
    INTP->quickKey = 0;
    INTP->copyBuffer = NULL;
}
