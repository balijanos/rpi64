/*
Copyright (C) 2015 BrainDead Software.
Pi64 based on load81 project see the copyright details below
*/

/******************************************************************************
* Copyright (C) 2012 Salvatore Sanfilippo.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifdef WINDOWS
       #include "usleep.h"
#endif

#include "pi64.h"
#include "framebuffer.h"
#include "commonEditor.h"
#include "interpreter.h"
#include "editor.h"

#include "sdlbindings.h"

#define NOTUSED(V) ((void) V)

struct globalConfig pi64;

/* =========================== Utility functions ============================ */

char *trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace(*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}

/* Return the UNIX time in microseconds */
long long ustime(void) {
    struct timeval tv;
    long long ust;
    
    gettimeofday(&tv, NULL);
    ust = ((long long)tv.tv_sec)*1000000;
    ust += tv.tv_usec;
    return ust;
}

/* Return the UNIX time in milliseconds */
long long mstime(void) {
    return ustime()/1000;
}

#ifdef WINDOWS
    #include <direct.h>
    #define GetCurrentDir _getcwd
#else
    #include <unistd.h>
    #define GetCurrentDir getcwd
 #endif

/* ========================= Lua helper functions ========================== */

/* Set a Lua global to the specified number. */
void setNumber(char *name, lua_Number n) {
    lua_pushnumber(pi64.L,n);
    lua_setglobal(pi64.L,name);
}

/* Get a Lua global containing a number. */
lua_Number getNumber(char *name) {
    lua_Number n;

    lua_getglobal(pi64.L,name);
    n = lua_tonumber(pi64.L,-1);
    lua_pop(pi64.L,1);
    return n;
}

/* Set a Lua global to the specified number. */
void setPointer(char *name, void * p) {
    lua_pushlightuserdata(pi64.L,p);
    lua_setglobal(pi64.L,name);
}

/* Set a Lua global to the specified string. */
void setString(char *name, char * str) {
    lua_pushstring(pi64.L,str);
    lua_setglobal(pi64.L,name);
}


/*  update the table_name[] table, at index, by given field, to num
    e.g. table_name[1].x = 100
    stack:
        table_name              global
            [1]                 index
                .x              field
                    = 100       number
*/
void setArrayFieldNumber( char *table_name, int index, char *field, int number)
{
    lua_getglobal(pi64.L, table_name);

    /* Create new if needed */
    if (lua_isnil(pi64.L,-1)) {
        lua_pop(pi64.L,1);
        lua_newtable(pi64.L);
        lua_setglobal(pi64.L, table_name);
        lua_getglobal(pi64.L, table_name);
    }

    /* lua: table_named[index].field = value  */ 
    if (lua_istable(pi64.L, -1)) {
        lua_pushnumber(pi64.L, index);
        /* get table for modification */
        lua_gettable(pi64.L, -2);    
        lua_pushstring(pi64.L, field);
        lua_pushnumber(pi64.L, number);
        lua_settable(pi64.L, -3);
    }

    lua_pop(pi64.L, 2);
}

/* 
    update the table_name[] table, .name field
    e.g. table_name[1].name = "nub0"
    stack:
        table_name              global
            [1]                 joynum
                .name           field
                    = "nub0"    value
*/
void setArrayFieldString(char *table_name, int index, char *field, const char *value)
{
    lua_getglobal(pi64.L, table_name);
    if (lua_isnil(pi64.L,-1)) {
        lua_pop(pi64.L,1);
        lua_newtable(pi64.L);
        lua_setglobal(pi64.L,table_name);
        lua_getglobal(pi64.L,table_name);
    }
 
    if (lua_istable(pi64.L, -1)) {
        lua_pushnumber(pi64.L, index);
        /* get table for modification */
        lua_gettable(pi64.L, -2);
        lua_pushstring(pi64.L, field);
        lua_pushstring(pi64.L, value);
        lua_settable(pi64.L, -3);
    }

    lua_pop(pi64.L, 2);
}


/* Set a Lua global table field to the value on the top of the Lua stack. */
void setTableField(char *name, char *field) {
    lua_getglobal(pi64.L,name);          /* Stack: val table */
    /* Create the table if needed */
    if (lua_isnil(pi64.L,-1)) {
        lua_pop(pi64.L,1);               /* Stack: val */
        lua_newtable(pi64.L);            /* Stack: val table */
        lua_setglobal(pi64.L,name);      /* Stack: val */
        lua_getglobal(pi64.L,name);      /* Stack: val table */
    }
    /* Set the field */
    if (lua_istable(pi64.L,-1)) {
        lua_pushstring(pi64.L,field);    /* Stack: val table field */
        lua_pushvalue(pi64.L,-3);        /* Stack: val table field val */
        lua_settable(pi64.L,-3);         /* Stack: val table */
    }
    lua_pop(pi64.L,2);                   /* Stack: (empty) */
}

void setTableFieldNumber(char *name, char *field, lua_Number n) {
    lua_pushnumber(pi64.L,n);
    setTableField(name,field);
}

void setTableFieldString(char *name, char *field, char *s) {
    lua_pushstring(pi64.L,s);
    setTableField(name,field);
}

/* Set the error string and the error line number. */
void programError(const char *e) {
    int line = 0;
    char *p;
    char str[15];    
    if ((p = strchr(e,':')) != NULL)
        line = atoi(p+1);
    // editorSetError(e,line);
    sprintf(str, "ERROR AT: %d", line);
    interpreterAppendRow(str);
    setString("LASTERROR",(char *)e);
    pi64.luaerr = 1;
}


/* ========================== Events processing ============================= */

void setup(void) {
    lua_getglobal(pi64.L,"setup");
    if (!lua_isnil(pi64.L,-1)) {
        if (lua_pcall(pi64.L,0,0,0)) {
            programError(lua_tostring(pi64.L, -1));
        }
    } else {
        // lua_pop(pi64.L,1);
    }
}

void cleanup(void) {
    lua_getglobal(pi64.L,"cleanup");
    if (!lua_isnil(pi64.L,-1)) {
        if (lua_pcall(pi64.L,0,0,0)) {
            programError(lua_tostring(pi64.L, -1));
        }
    } else {
        // lua_pop(pi64.L,1);
    }
}

int draw(void) {    
    lua_getglobal(pi64.L,"draw");
    if (!lua_isnil(pi64.L,-1)) {
        if (lua_pcall(pi64.L,0,1,0)) {
            programError(lua_tostring(pi64.L, -1));
        }
        if (!lua_isnil(pi64.L,-1) && lua_tointeger(pi64.L,-1))
           return -1;
        else
            return 1;
    } else {
        lua_pop(pi64.L,1);
        return 0;
    }
}

/* Update the keyboard.pressed and mouse.pressed Lua table. */
void updatePressedState(char *object, char *keyname, int pressed) {
    lua_getglobal(pi64.L,object);         /* $keyboard */
    lua_pushstring(pi64.L,"pressed");     /* $keyboard, "pressed" */
    lua_gettable(pi64.L,-2);              /* $keyboard, $pressed */
    lua_pushstring(pi64.L,keyname);       /* $keyboard, $pressed, "keyname" */
    if (pressed) {
        lua_pushboolean(pi64.L,1);        /* $k, $pressed, "keyname", true */
    } else {
        lua_pushnil(pi64.L);              /* $k, $pressed, "keyname", nil */
    }
    lua_settable(pi64.L,-3);              /* $k, $pressed */
    lua_pop(pi64.L,2);
}

void keyboardEvent(SDL_KeyboardEvent *key, int down) {
    char *keyname = SDL_GetKeyName(key->keysym.sym);

    setTableFieldString("keyboard","state",down ? "down" : "up");
    setTableFieldString("keyboard","key",keyname);
    updatePressedState("keyboard",keyname,down);
}

void mouseMovedEvent(int x, int y, int xrel, int yrel) {
    setTableFieldNumber("mouse","x",x);
    setTableFieldNumber("mouse","y",pi64.height-1-y);
    setTableFieldNumber("mouse","xrel",xrel);
    setTableFieldNumber("mouse","yrel",-yrel);
}

void mouseButtonEvent(int button, int pressed) {
    char buttonname[32];
    
    snprintf(buttonname,sizeof(buttonname),"%d",button);
    updatePressedState("mouse",buttonname,pressed);
}

void joystickXMovedEvent(int joy_num, Sint16 x) { 
    if (joy_num < MAX_JOYSTICKS) {
        setArrayFieldNumber("joystick", joy_num, "x", x);
    }
}

void joystickYMovedEvent(int joy_num, Sint16 y) { 
    if (joy_num < MAX_JOYSTICKS) {
        setArrayFieldNumber("joystick", joy_num, "y", y);
    }
}
void joystickButtonEvent(int joy_num, int down)
{
    if (joy_num < MAX_JOYSTICKS) {
        setArrayFieldNumber("joystick", joy_num, "button", down);
    }    
}

void resetEvents(void) {
    setTableFieldString("keyboard","state","none");
    setTableFieldString("keyboard","key","");
}

void showFPS(void) {
    int elapsed_ms = mstime()-pi64.start_ms;
    char buf[64];

    if (!elapsed_ms) return;
    snprintf(buf,sizeof(buf),"FPS: %.2f",(float)(pi64.epoch*1000)/elapsed_ms);
    drawBox(pi64.fb,0,0,100,20,0,0,0,255,1);
    bfWriteString(pi64.fb,BitmapFont,0,0,buf,strlen(buf),128,128,128,255,0);
}

int processSdlEvents(void) {
    SDL_Event event;
    int n;
    SDLMod mod;
    
    resetEvents();
    while (SDL_PollEvent(&event)) {
        mod = event.key.keysym.mod;
        switch(event.type) {
        case SDL_KEYDOWN:
            switch(event.key.keysym.sym) {
            case SDLK_ESCAPE:
                if( mod & KMOD_RSHIFT || mod & KMOD_LSHIFT ) {
                    if (pi64.mode == RUNTIME) {
                       cleanup();
                       if (pi64.luaerr) return pi64.luaerr;
                    }
                    return 1;
                }
                else 
                     keyboardEvent(&event.key,1);
                break;
            default:
                keyboardEvent(&event.key,1);
                break;
            }
            break;
        case SDL_KEYUP:
            keyboardEvent(&event.key,0);
            break;
        case SDL_MOUSEMOTION:
            mouseMovedEvent(event.motion.x,event.motion.y,
                       event.motion.xrel,event.motion.yrel);
            break;
        case SDL_MOUSEBUTTONDOWN:
            mouseButtonEvent(event.button.button,1);
            break;
        case SDL_MOUSEBUTTONUP:
            mouseButtonEvent(event.button.button,0);
            break;
        case SDL_JOYAXISMOTION:  /* Handle Joystick Motion */
            if( event.jaxis.axis == 0) { /* x-axis */
                joystickXMovedEvent(event.jaxis.which + 1, event.jaxis.value);  /* C vs. Lua offsets */
            }
            if( event.jaxis.axis == 1) { /* y-axis */
                joystickYMovedEvent(event.jaxis.which + 1, event.jaxis.value);  /* C vs. Lua offsets */
            }
        break;
        case SDL_JOYBUTTONUP:  /* Handle Joystick Button Presses */
            joystickButtonEvent(event.jbutton.which + 1, 0);
        break;
        case SDL_JOYBUTTONDOWN:  /* Handle Joystick Button Presses */
            joystickButtonEvent(event.jbutton.which + 1, 1);
        break;

        case SDL_QUIT:
            exit(0);
            break;
        }
        /* If the next event to process is of type KEYUP or
         * MOUSEBUTTONUP we want to stop processing here, so that
         * a fast up/down event be noticed by Lua. */
        if (SDL_PeepEvents(&event,1,SDL_PEEKEVENT,SDL_ALLEVENTS)) {
            if (event.type == SDL_KEYUP ||
                event.type == SDL_MOUSEBUTTONUP)
                break; /* Go to lua before processing more. */
        }
    }
    /* Call the setup function, only the first time. */
    if (pi64.mode == RUNTIME || pi64.mode == GRAPHIC) {
       if (pi64.epoch == 0) {         
         setup();
         if (pi64.luaerr) return pi64.luaerr;
       }
       /* Call the draw function at every iteration.  */
       n = draw();
       if (n==0)
          return 1;
       else if (n==-1) { // Software stop
          if (pi64.mode == RUNTIME || pi64.mode == GRAPHIC) {
             cleanup();
             if (pi64.luaerr) 
                return pi64.luaerr;
             pi64.mode = INTERPRETER;
             return 1;
          }
          return 1;
       }
       pi64.epoch++;
    }
    /* Refresh the screen */
    if (pi64.opt_show_fps) showFPS();
    SDL_Flip(pi64.fb->screen);
    /* Wait some time if the frame was produced in less than 1/FPS seconds. */
    SDL_framerateDelay(&pi64.fb->fps_mgr);
    /* Stop execution on error */
    return pi64.luaerr;
}

/* =========================== Initialization ============================== */

void initConfig(void) {
    pi64.width = DEFAULT_WIDTH;
    pi64.height = DEFAULT_HEIGHT;
    pi64.bpp = DEFAULT_BPP;
    pi64.fps = 30;
    pi64.r = 255;
    pi64.g = pi64.b = 0;
    pi64.alpha = 255;
    pi64.filled = 1;
    pi64.L = NULL;
    pi64.luaerr = 0;
    pi64.opt_show_fps = 0;
    pi64.opt_full_screen = 0;
    // pi64.filename = NULL;
    pi64.mode = INTERPRETER;
    srand(mstime());
    
    syscolor cl;
    cl.r = 120;
    cl.g = 120;
    cl.b = 120;
    cl.a = 255;    
    sysscheme[RPI_FONT]=cl;
    int i;
    for (i=0;i<HL_COLORS;i++) hlscheme[i]=cl;
    cl.a = 128; 
    sysscheme[RPI_EDITOR_CURSOR]=cl;
    sysscheme[RPI_SHELL_CURSOR]=cl;
    cl.r = 0;
    cl.g = 0;
    cl.b = 0;
    cl.a = 255;
    sysscheme[RPI_EDITOR]=cl;
    sysscheme[RPI_SHELL]=cl;
    sysscheme[RPI_FRAME]=cl;
}

/* Load the editor program into Lua. Returns 0 on success, 1 on error. */
int loadProgram(char *filename) {
    int buflen;
    char *buf = ceditorRowsToString(E,&buflen);   
    setString("LASTERROR",""); 
    // if (luaL_loadbuffer(pi64.L,buf,buflen,pi64.filename)) {
    if (luaL_loadbuffer(pi64.L,buf,buflen,filename)) {
        programError(lua_tostring(pi64.L, -1));
        free(buf);
        return 1;
    }
    free(buf);
    if (lua_pcall(pi64.L,0,0,0)) {
        programError(lua_tostring(pi64.L, -1));        
        return 1;
    } 
    pi64.luaerr = 0;
    // editorClearError();    
    return 0;
}

void initScreen(void) {
    pi64.fb = createFrameBuffer(pi64.width,pi64.height,
                               pi64.bpp,pi64.opt_full_screen);
}

void initJoysticks(frameBuffer *fb) {
    int cur_joy;
    for(cur_joy=0; cur_joy < MAX_JOYSTICKS; cur_joy++ ) {
        fb->joysticks[cur_joy] = NULL;
    }
}

void resetJoysticks(frameBuffer *fb) { 
    int cur_joy, sdl_joys, num_joys;
    char joyscript[70];

    /* Initialize Joysticks */
    SDL_JoystickEventState(SDL_ENABLE);


    for(sdl_joys = SDL_NumJoysticks(), cur_joy=0, num_joys=0; cur_joy < sdl_joys; cur_joy++ ) {
    
        if (cur_joy == 0) {
            snprintf(joyscript, sizeof(joyscript),
                "for jn = 1, %d, 1 do joystick[jn]={x=0;y=0;name=nil;button=0}; end ", sdl_joys);
            luaL_loadbuffer(pi64.L,joyscript,strlen(joyscript),"joyscript");
            lua_pcall(pi64.L,0,0,0);
        }

        if (fb->joysticks[cur_joy] != NULL)
            SDL_JoystickClose( fb->joysticks[cur_joy]);

        if (cur_joy < MAX_JOYSTICKS) {
            fb->joysticks[cur_joy] = SDL_JoystickOpen(cur_joy);

            if (fb->joysticks[cur_joy] != NULL) {
                setArrayFieldString("joystick", cur_joy + 1, "name", SDL_JoystickName(cur_joy));
                setArrayFieldNumber("joystick", cur_joy + 1, "axes", SDL_JoystickNumAxes(fb->joysticks[cur_joy]));
                setArrayFieldNumber("joystick", cur_joy + 1, "trackballs", SDL_JoystickNumBalls(fb->joysticks[cur_joy]));
                setArrayFieldNumber("joystick", cur_joy + 1, "hats", SDL_JoystickNumHats(fb->joysticks[cur_joy]));
                setArrayFieldNumber("joystick", cur_joy + 1, "buttons", SDL_JoystickNumButtons(fb->joysticks[cur_joy]));
                setArrayFieldNumber("joystick", cur_joy + 1, "x", 0);
                setArrayFieldNumber("joystick", cur_joy + 1, "y", 0);
                num_joys ++;
            }
        }
    }

    setTableFieldNumber("joystick", "count", num_joys);
}


void resetProgram(void) {
    char *initscript =
        "keyboard={}; keyboard['pressed']={};" \
        "mouse={}; mouse['pressed']={};" \
        "joystick={}; " \
        "sprites={}";

    pi64.epoch = 0;

    setPointer("SCREEN", pi64.fb->screen);
    setNumber("WIDTH",pi64.width);
    setNumber("HEIGHT",pi64.height);
    // setNumber("BORDER",pi64.border);
    luaL_loadbuffer(pi64.L,initscript,strlen(initscript),"initscript");
    lua_pcall(pi64.L,0,0,0);

    /* Make sure that mouse parameters make sense even before the first
     * mouse event captured by SDL */
    setTableFieldNumber("mouse","x",0);
    setTableFieldNumber("mouse","y",0);
    setTableFieldNumber("mouse","xrel",0);
    setTableFieldNumber("mouse","yrel",0);

    /* Reset joysticks */
    resetJoysticks(pi64.fb);
    
    /* Register API */
    // luaopen_bit32(pi64.L);
    SDLBindings(pi64.L);
    initSpriteEngine(pi64.L);    
    
    // Simple physics    
    // luaopen_spl(pi64.L);

    /* Start with a black screen */
    fillBackground(pi64.fb,0,0,0);
}

int resetLua() { 
    if (pi64.L) lua_close(pi64.L);
    pi64.L = lua_open();
    luaL_openlibs(pi64.L);
    // disable libs below
    // setString("io",NULL);
        
    setString("LASTERROR","");
    setString("_VERSION",PI64_VERSION);
    return 0;
}

int resetRom(char *romname) {     
    clearKeywords();
    char romfile[1024]="\0";
    strcpy(romfile,ROMROOT);
    strcat(romfile,romname);
    /* Register interpreter API */
    LUA_INTP_Bindings(pi64.L);
    if (luaL_loadfile(pi64.L,romfile)) {
        programError(lua_tostring(pi64.L, -1));
        return 1;
    }
    if (lua_pcall(pi64.L,0,0,0)) {
        programError(lua_tostring(pi64.L, -1));
        return 1;
    }
    pi64.luaerr = 0;
    // editorClearError();
    return 0;
}

/* ================================= Main ================================== */

void showCliHelp(void) {
    fprintf(stderr, "Usage: pi64 [options] [romprogram.lua]\n"
"  --width <pixels>       Set screen width\n"
"  --height <pixels>      Set screen height\n"
"  --full                 Enable full screen mode\n"
"  --bpp                  SDL bit per pixel setting (default=24, 0=hardware)\n"
"  --fps                  Show frames per second\n"
"  --help                 Show this help screen\n"
           );
    exit(1);
}

void parseOptions(int argc, char **argv) {
    int j;
    pi64.romname = strdup(ROMNAME);
    for (j = 1; j < argc; j++) {
        char *arg = argv[j];
        int lastarg = j == argc-1;

        if (!strcasecmp(arg,"--fps")) {
            pi64.opt_show_fps = 1;
        } else if (!strcasecmp(arg,"--full")) {
            pi64.opt_full_screen = 1;
        } else if (!strcasecmp(arg,"--width") && !lastarg) {
            pi64.width = atoi(argv[++j]);
        } else if (!strcasecmp(arg,"--height") && !lastarg) {
            pi64.height = atoi(argv[++j]);
        } else if (!strcasecmp(arg,"--bpp") && !lastarg) {
            pi64.bpp = atoi(argv[++j]);
            if (pi64.bpp != 8 && pi64.bpp != 16 && pi64.bpp != 24 && pi64.bpp != 32
                && pi64.bpp != 0)
            {
                fprintf(stderr,"Invalid bit per pixel. Try with: 8, 16, 24, 32 or 0 for auto-select the hardware default.");
                exit(1);
            }
        } else if (!strcasecmp(arg,"--help")) {
            showCliHelp();
        } else {
            if (arg[0] != '-') {
                pi64.romname = arg;
            } // else {
            //     fprintf(stderr,
            //        "Unrecognized option or missing argument: %s\n\n", arg);
            //    showCliHelp();
            //}
        }
    }
    // if (pi64.filename == NULL) {
    //    fprintf(stderr,"No Lua program filename specified.\n\n");
    //    showCliHelp();
    // }
}

int main(int argc, char **argv) {
    NOTUSED(argc);
    NOTUSED(argv);

    initConfig();
    parseOptions(argc,argv);
    initScreen();
    initJoysticks(pi64.fb);    
    
    char appPath[FILENAME_MAX];
    GetCurrentDir(appPath, sizeof(appPath));    
    
    INTP = malloc(sizeof(commonEditorConfig));
    E = malloc(sizeof(commonEditorConfig));
        
    pi64.resetFlag = 1;
    pi64.border = 30;
    BORDER_T = pi64.border;
    BORDER_B = pi64.border;
    BORDER_L = pi64.border+6;
    BORDER_R = pi64.border+6;
    POWEROFF_BUTTON_X = (pi64.fb->width-18);
    POWEROFF_BUTTON_Y = 18;
    // SAVE_BUTTON_X = (pi64.fb->width-BORDER_R-13);
    SAVE_BUTTON_X = (pi64.fb->width-18);
    SAVE_BUTTON_Y = (pi64.fb->height-16);
    
    while(1) {   
         
        // soft restart computer
        if (pi64.resetFlag) {
            pi64.resetFlag = 0;
            chdir(appPath);
            resetLua();            
            // pre init
            initEditor(pi64.fb,BORDER_T,BORDER_B,BORDER_L,BORDER_R);
            initInterpreter(pi64.fb,BORDER_T,BORDER_B,BORDER_L,BORDER_R);                          
            // editorNew();
            resetProgram();
            resetRom(pi64.romname);             
            // check ROM autoload ....
            if (E->filename!=NULL) {
               pi64.mode=RUNTIME;               
            } else               
              pi64.mode=INTERPRETER;
            
        }
        //INTERPRETER MODE                
        if (pi64.mode==INTERPRETER) {
              interpreterRun();           
              if (interpreterIsQuickCommand()) {
                 ceditorCurrentRowToString(INTP, &INTP->command);
                 execQuickCommand(interpreterGetQuickCommand(),interpreterGetCommand());
              } else 
              if (interpreterIsCommand()) {
                 execCommand(interpreterGetCommand());                 
              } else {
                 pi64.mode=EDITOR;
              }
        } else
        if (pi64.mode==EDITOR) {         
           editorRun();
           pi64.mode=INTERPRETER;                      
        } else 
        if (pi64.mode==RUNTIME) { 
           resetProgram();
           loadProgram(E->filename);
           if (!pi64.luaerr) {
             SDL_setFramerate(&pi64.fb->fps_mgr,pi64.fps);
             pi64.start_ms = mstime();
             while(!processSdlEvents());              
           } 
           pi64.mode=INTERPRETER;
        } else
        if (pi64.mode==GRAPHIC) {                
           if (!pi64.luaerr) {
             SDL_setFramerate(&pi64.fb->fps_mgr,pi64.fps);
             pi64.start_ms = mstime();
             while(!processSdlEvents());              
           } 
           pi64.mode=INTERPRETER;
        }
    }
    return 0;
}
