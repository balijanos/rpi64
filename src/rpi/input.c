#include "commonEditor.h"
#include "editor.h"
#include "pi64.h"
#include "interpreter.h"

#ifdef WINDOWS
       #include "usleep.h"
#endif

#include <time.h>
#include <unistd.h>

int inputEvents(int max) {
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
            // case SDLK_PAGEUP:
            // case SDLK_PAGEDOWN:
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
                return -1;
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
                if (INTP->modifiers & CTRL_MASK) {  
                   INTP->key[j].counter--;                   
                   switch(j) {
                   // CTRL+B
                        case 98:                   
                             ceditorDelCharsToCursor(INTP);
                             break;                                               
                   // CTRL+E
                        case 101:                   
                             ceditorDelCharsFromCursor(INTP);
                             break;                         
                                                                                
                        default:                              
                             ceditorInsertChar(INTP, INTP->key[j].translation);
                             break;
                   }                
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
    
    return ( interpreterIsCommand() || interpreterIsQuickCommand() );
}

char *textInput() {
    int j,ret;      
    char *inp;
    for (j = 0; j < KEY_MAX; j++) {
        INTP->key[j].counter = 0;
    }
    int mode = pi64.mode;
    pi64.mode = INTERPRETER;
    INTP->lastevent = time(NULL);    
    while(!(ret=inputEvents(0)));
    if (ret == -1) {
           return interpreterGetCommand();
    }
    pi64.mode = mode;
    return NULL;
}
