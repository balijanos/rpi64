#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "framebuffer.h"
#include "commonEditor.h"
#include "pi64.h"
#include "input.h"

#define DEBUGLOG 0
/* ------------------------- LUA bindings --------------------------------------- */

void rom_interpret(char *cmd) {
    lua_getglobal(pi64.L,"interpret");
    if (!lua_isnil(pi64.L,1)) {
        lua_pushstring(pi64.L, cmd);  
        if (lua_pcall(pi64.L,1,2,0)) {             
        }
    } else {
        lua_pop(pi64.L,1);
    }
}

// v1.1
void rom_quickcmd(int cmd, char *str) {
    lua_getglobal(pi64.L,"quickcmd");
    if (!lua_isnil(pi64.L,1)) {
        lua_pushinteger(pi64.L, cmd);  
        if (str != NULL) 
           lua_pushstring(pi64.L, str);  
        else
           lua_pushnil(pi64.L);  
        if (lua_pcall(pi64.L,2,2,0)) {             
        }
    } else {
        lua_pop(pi64.L,1);
    }
}

/* Load the editor program into Lua. Returns 0 on success, 1 on error. */
int execCommand(char *cmd) {
    if (cmd==NULL || strlen((char *)trimwhitespace(cmd))==0) return 0;
    rom_interpret(cmd);
    char *msg = (char *)lua_tostring(pi64.L, -1);    
    if (lua_gettop(pi64.L)==2 && lua_tonumber(pi64.L, -2)==1) {
       if (msg)
          interpreterAppendRow(msg);  
    } else if (lua_tonumber(pi64.L, 1)==0 && msg != NULL)  {
       interpreterAppendRow(msg);  
    }
    
    pi64.luaerr = 0;
    free(cmd);
    return 0;
}

/* Load the editor program into Lua. Returns 0 on success, 1 on error. */
int execQuickCommand(int qcmd, char *str) {
    if (qcmd==0) return 0;
    rom_quickcmd(qcmd,str);
    char *msg = (char *)lua_tostring(pi64.L, -1);    
    if (lua_gettop(pi64.L)==2 && lua_tonumber(pi64.L, -2)==1) {
       if (msg)
          interpreterAppendRow(msg);  
    } else if (lua_tonumber(pi64.L, 1)==0 && msg != NULL)  {
       interpreterAppendRow(msg);  
    }    
    pi64.luaerr = 0;
    return 0;
}

int dirExists(char *pathname) {
    struct stat info;
    if( stat( pathname, &info ) != 0 )
        return 0;
    else if( info.st_mode & S_IFDIR )  // S_ISDIR() doesn't exist on my windows 
         return 1;
    else
        return 0;
}

int fileExists(char* file) {
    struct stat buf;
    return (stat(file, &buf) == 0);
}

void debuglog(char *s, char *f) {
     if ( DEBUGLOG ) {
         fprintf(stderr, f, s);
     }
}

void interpreterSetKeyword(char *keyword) {
     int i=0;
     while (keywords[i] && i<255) {
           i++;
     }
     if (i<255) {
        keywords[i] = strdup(keyword); 
     }
}

void clearKeywords(void) {
     int i=0;
     while (keywords[i] && i<255) {
           free(keywords[i]);
           keywords[i] = NULL;
           i++;
     }
}

/*
Raise error if rom call string param is NULL
*/
void checkArgNull(lua_State *L) {
     if ( lua_tostring(L, 1)==NULL ) {
       luaL_error(L,"NULL VALUE");
     }
}

int luai_print(lua_State *L) {    
    debuglog((char *)lua_tostring(L, -1),"[PRINT] %s\n");
    checkArgNull(L);
    char *iscr = (char *)lua_tostring(L, 1);
    char *msg = (char *)lua_tostring(L, -1);
    if (strchr(iscr, '\r')!=NULL) {
       interpreterUpdateRow(msg);
    } else {
       interpreterAppendRow(msg);
    }    
    interpreterDraw();
    return 0;
}

int luai_input(lua_State *L) {    
    debuglog("","[INPUT]\n");
    lua_pushstring(L,textInput()); 
    return 1;
}


int luai_open(lua_State *L) {
    checkArgNull(L);
    if (dirExists((char *)lua_tostring(L, -1))) {
       debuglog((char *)lua_tostring(L, -1),"[OPEN] %s\n");
    }
    else {
       debuglog((char *)lua_tostring(L, -1),"[OPEN ERR] %s\n");
       lua_remove(L,-1); 
       lua_pushnil(L);
    }
    return 1;
}

int luai_load(lua_State *L) {
    checkArgNull(L);
    if (fileExists((char *)lua_tostring(L, -1))) {
         if (editorOpen((char *)lua_tostring(L, 1))) {
           debuglog((char *)lua_tostring(L, 1),"[LOAD] %s\n");                         
           lua_remove(L,-1); 
           lua_pushnil(L);
         }
    } else {
          debuglog((char *)lua_tostring(L, -1),"[LOAD ERR] %s\n");   
          lua_remove(L,-1); 
          lua_pushnil(L);
    }
    return 1;
}

int luai_save(lua_State *L) { 
    debuglog((char *)lua_tostring(L, 1),"[SAVE] %s\n");
    checkArgNull(L);         
    if (editorSave((char *)lua_tostring(L, 1))) {
        lua_remove(L,-1); 
        lua_pushnil(L);
    }
    return 1;
}

int luai_new(lua_State *L) { 
    debuglog("","[NEW] \n");    
    if (editorNew()) {
        lua_remove(L,-1); 
        lua_pushnil(L);
    }
    return 1;
}

/* BUFFER to file */
int luai_copy(lua_State *L) {
    char filepath[1024];   
    FILE* pFile;
    size_t n;
    void *buffer;
    debuglog((char *)lua_tostring(L, -3),"[COPY] %s\n");
    checkArgNull(L);
    if (lua_isnil(L, -3) || lua_isnil(L, -1) || !lua_isnumber(L, -1) ) return 1;             
    n =  lua_tonumber(L, -1);
    buffer = (void *)lua_tostring(L,-2);            
    strcpy(filepath,lua_tostring(L, -3)); 
    /* Write buffer to disk. */
    pFile = fopen(filepath,"wb");
    if (pFile){
       n = fwrite(buffer, n, 1, pFile);
       lua_pushnumber(L,n);
    }
    else{
       lua_pushnil(L);
       return 1;
    }
    fclose(pFile);    
    return 1;
}

/* copy file */
int luai_filecopy(lua_State *L) {
    debuglog((char *)lua_tostring(L,1),"[COPYFILE] %s\n");
    if (lua_isnil(L, 1) || lua_isnil(L, 2)) {
       lua_pushnil(L);
       return 1;
    }
    FILE *fd1 = fopen(lua_tostring(L,1), "rb");
    if(!fd1) {
       lua_pushnil(L);
       return 1;
    }
    FILE *fd2 = fopen(lua_tostring(L,2), "wb");
     if(!fd2) {
       fclose(fd1);
       lua_pushnil(L);
       return 1;
    }
    size_t l1;
    size_t lsum = 0;
    unsigned char buffer[8192]; 

    while((l1 = fread(buffer, 1, sizeof buffer, fd1)) > 0) {
       size_t l2 = fwrite(buffer, 1, l1, fd2);
       lsum += l2;
       if (l2<0 || l2 < l1) {
          lua_pushnil(L);
          fclose(fd1);
          fclose(fd2);   
          return 1;
       }
    }
    lua_pushnumber(L,lsum);
    fclose(fd1);
    fclose(fd2);   
    return 1;
}

int luai_clear(lua_State *L) {
    debuglog("","[CLEAR]\n");
    initInterpreter(pi64.fb,BORDER_T,BORDER_B,BORDER_L,BORDER_R); 
    return 0;
}

int luai_reset(lua_State *L) {
    debuglog((char *)lua_tostring(L, 1),"[RESET] %s\n");
    checkArgNull(L);
    char romname[256] = "\0";
    strcpy(romname,ROMROOT);
    strcat(romname,(char *)lua_tostring(L, 1));    
    if ( fileExists(romname)==1 ) {
       debuglog((char *)romname,"[ROMNAME] %s\n");
       if (pi64.romname != NULL)
          free(pi64.romname);
       pi64.romname = strdup(lua_tostring(L, 1)); 
    } else pi64.romname = strdup(ROMNAME); 
    pi64.resetFlag = 1;
    return 0;
}

int luai_run(lua_State *L) {    
    debuglog("","[RUN]\n");
    pi64.mode=RUNTIME;
    pi64.epoch = 0;
    return 0;
}

int luai_quit(lua_State *L) {    
    debuglog("","[QUIT]\n");
    exit(3);
    return 0;
}

int luai_delete(lua_State *L) {
    char filepath[256];
    debuglog((char *)lua_tostring(L, 1),"[DELETE] %s\n");
    checkArgNull(L);   
    strcpy(filepath,lua_tostring(L, 1)); 
    if (remove((char *)filepath)){       
        debuglog((char *)filepath,"[DELETE ERR] %s\n");    
        lua_remove(L,-1); 
        lua_pushnil(L);
    }
    return 1;
}
// interpreter private funcs

int luai_errlog(lua_State *L) {
    debuglog((char *)lua_tostring(L, 1),"[ROM] %s\n");
    checkArgNull(L);
    return 0;
}

int luai_stdlog(lua_State *L) {    
    debuglog((char *)lua_tostring(L, 1),"[LOG] %s\n");
    checkArgNull(L);
    return 0;
}

int luai_addkeyword(lua_State *L) {
    checkArgNull(L);
    interpreterSetKeyword((char *)lua_tostring(L, 1));
    return 0;
}


/* Evaluate the loaded expression.
 * 
 * Returns the result or 0 on error.
 */
int luai_eval(lua_State *L)
{
    int err;
    char *buf;
    char *expr;
    
    checkArgNull(L);
    
    expr = (char *)lua_tostring(L,-1);
    debuglog(expr,"[EVAL] %s\n");
    
    buf = malloc(strlen(expr)+8);    
    strcpy(buf, "return ");
    strcat(buf, expr);

    err = luaL_loadstring(L,buf);
    free(buf);
    
    if (err) {  
       lua_pushnil(L);
       return 2;
    }
    int cookie = luaL_ref(L, LUA_REGISTRYINDEX);    
    lua_rawgeti(L, LUA_REGISTRYINDEX, cookie);
    
    err = lua_pcall(L,0,1,0);
    if (err) {
        lua_pushnil(L);
        return 2;
    } 
    luaL_unref(L, LUA_REGISTRYINDEX, cookie);    
    lua_pushnumber(L,0);
    return 2;
}

/* Interprete the loaded expression.
 * 
 * Returns the result or 0 on error.
 */
int luai_interpret(lua_State *L)
{
    int err;
    char *buf;
    char *expr;
    
    checkArgNull(L);
    
    expr = (char *)lua_tostring(L,-1);
    debuglog(expr,"[INTPEVAL] %s\n");
    
    buf = malloc(strlen(expr)+1);    
    strcpy(buf, expr);

    err = luaL_loadstring(L,buf);
    free(buf);

    if (err) {
       return 0;
    }
    int cookie = luaL_ref(L, LUA_REGISTRYINDEX);   
    lua_rawgeti(L, LUA_REGISTRYINDEX, cookie);
    err = lua_pcall(L,0,1,0);
    if (err) {
        return 0;
    } 
    luaL_unref(L, LUA_REGISTRYINDEX, cookie);    
    lua_pushnumber(L,0);
    return 2;
}

int luai_mode(lua_State *L) { 
    checkArgNull(L);
    int m = lua_tointeger(L,-1); 
    char str[15];
    sprintf(str, "%d", m);  
    debuglog(str,"[MODE] %s\n");
    pi64.mode=INTERPRETER;
	switch(m)
	{
		case(1):
			pi64.mode=INTERPRETER;
		break;
		case(2):
			pi64.mode=EDITOR;
		break;
		case(3):
			pi64.mode=RUNTIME;
		break;		
		case(4):
			pi64.mode=GRAPHIC;
		break;		
    }    
    pi64.epoch = 0;
    return 0;
}

void LUA_INTP_Bindings(lua_State *L) {
          
    lua_pushcfunction(L,luai_print);
    lua_setglobal(L,"INTP_print");   // append
    lua_pushcfunction(L,luai_input);
    lua_setglobal(L,"INTP_input");
    lua_pushcfunction(L,luai_open);
    lua_setglobal(L,"INTP_open");
    lua_pushcfunction(L,luai_load);
    lua_setglobal(L,"INTP_load");
    lua_pushcfunction(L,luai_save);
    lua_setglobal(L,"INTP_save");
    lua_pushcfunction(L,luai_new);
    lua_setglobal(L,"INTP_new");
    lua_pushcfunction(L,luai_copy);
    lua_setglobal(L,"INTP_copy");
    lua_pushcfunction(L,luai_filecopy);
    lua_setglobal(L,"INTP_filecopy");
    lua_pushcfunction(L,luai_delete);
    lua_setglobal(L,"INTP_delete");
    lua_pushcfunction(L,luai_clear);
    lua_setglobal(L,"INTP_clear");
    lua_pushcfunction(L,luai_reset);
    lua_setglobal(L,"INTP_reset");
    lua_pushcfunction(L,luai_run);
    lua_setglobal(L,"INTP_run");
    lua_pushcfunction(L,luai_quit);
    lua_setglobal(L,"INTP_quit");
    lua_pushcfunction(L,luai_eval);
    lua_setglobal(L,"INTP_eval");     
    lua_pushcfunction(L,luai_interpret);
    lua_setglobal(L,"INTP_ieval");       
    lua_pushcfunction(L,luai_errlog);
    lua_setglobal(L,"INTP_errlog");
    lua_pushcfunction(L,luai_stdlog);
    lua_setglobal(L,"INTP_stdlog");  
    lua_pushcfunction(L,luai_addkeyword);
    lua_setglobal(L,"INTP_addkeyword"); 
    lua_pushcfunction(L,luai_mode);
    lua_setglobal(L,"INTP_mode"); 

}

