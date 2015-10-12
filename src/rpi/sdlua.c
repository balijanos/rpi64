#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "framebuffer.h"
#include "commonEditor.h"
#include "pi64.h"

/* ============================= Lua bindings ============================== */
int fillBinding(lua_State *L) {
    pi64.r = lua_tonumber(L,-4);
    pi64.g = lua_tonumber(L,-3);
    pi64.b = lua_tonumber(L,-2);
    pi64.alpha = lua_tonumber(L,-1) * 255;
    if (pi64.r < 0) pi64.r = 0;
    if (pi64.r > 255) pi64.r = 255;
    if (pi64.g < 0) pi64.g = 0;
    if (pi64.g > 255) pi64.g = 255;
    if (pi64.b < 0) pi64.b = 0;
    if (pi64.b > 255) pi64.b = 255;
    if (pi64.alpha < 0) pi64.alpha = 0;
    if (pi64.alpha > 255) pi64.alpha = 255;
    return 0;
}

int filledBinding(lua_State *L) {
  pi64.filled = lua_toboolean(L,-1);
  return 0;
}

int rectBinding(lua_State *L) {
    int x,y,w,h;

    x = lua_tonumber(L,-4);
    y = lua_tonumber(L,-3);
    w = lua_tonumber(L,-2);
    h = lua_tonumber(L,-1);
    drawBox(pi64.fb,x,y,x+(w-1),y+(h-1),pi64.r,pi64.g,pi64.b,pi64.alpha,pi64.filled);
    return 0;
}

int ellipseBinding(lua_State *L) {
    int x,y,rx,ry;

    x = lua_tonumber(L,-4);
    y = lua_tonumber(L,-3);
    rx = lua_tonumber(L,-2);
    ry = lua_tonumber(L,-1);
    drawEllipse(pi64.fb,x,y,rx,ry,pi64.r,pi64.g,pi64.b,pi64.alpha,pi64.filled);
    return 0;
}

int triangleBinding(lua_State *L) {
    int x1,y1,x2,y2,x3,y3;

    x1 = lua_tonumber(L,-6);
    y1 = lua_tonumber(L,-5);
    x2 = lua_tonumber(L,-4);
    y2 = lua_tonumber(L,-3);
    x3 = lua_tonumber(L,-2);
    y3 = lua_tonumber(L,-1);
    drawTriangle(pi64.fb,x1,y1,x2,y2,x3,y3,pi64.r,pi64.g,pi64.b,pi64.alpha,pi64.filled);
    return 0;
}

int lineBinding(lua_State *L) {
    int x1,y1,x2,y2;

    x1 = lua_tonumber(L,-4);
    y1 = lua_tonumber(L,-3);
    x2 = lua_tonumber(L,-2);
    y2 = lua_tonumber(L,-1);
    drawLine(pi64.fb,x1,y1,x2,y2,pi64.r,pi64.g,pi64.b,pi64.alpha);
    return 0;
}

int textBinding(lua_State *L) {
    int x,y,k;
    const char *s;
    size_t len;

    x = lua_tointeger(L,1);
    y = lua_tointeger(L,2);
    s = lua_tolstring(L,3,&len);
    if (!s) return 0;
    k = 0;
    if (lua_isnumber(L,5))
       k = lua_tointeger(L,5);
    if (lua_isnil(L,4) || !lua_toboolean(L,4)) {
        bfWriteString(pi64.fb,BitmapFont,x,y,s,len,pi64.r,pi64.g,pi64.b,pi64.alpha,k);
    } else {
        bfWriteString(pi64.fb,UBitmapFont,x,y,s,len,pi64.r,pi64.g,pi64.b,pi64.alpha,k);
    }
            
    return 0;
}

int setFPSBinding(lua_State *L) {
    pi64.fps = lua_tonumber(L,-1);

    if (pi64.fps <= 0) pi64.fps = 1;
    SDL_setFramerate(&pi64.fb->fps_mgr,pi64.fps);
    return 0;
}

int backgroundBinding(lua_State *L) {
    int r,g,b;

    r = lua_tonumber(L,-3);
    g = lua_tonumber(L,-2);
    b = lua_tonumber(L,-1);
    fillBackground(pi64.fb,r,g,b);
    return 0;
}

int polygonBinding(lua_State *L) {
  Sint16* polyBufferX;
  Sint16* polyBufferY;

  if (!(lua_gettop(L) == 2 && lua_istable(L,-1) && lua_istable(L,-2))) {
      programError("Invalid arguments for polygon");
      return 0;
  }

  int size = (int)lua_objlen(L,-1), i=0;
  polyBufferY = (Sint16*)malloc(size * sizeof(Sint16));
  lua_pushnil(L);
  while(lua_next(L,-2) != 0) {
    polyBufferY[i++] = (Sint16)lua_tonumber(L,-1);
    lua_pop(L,1);
    if (i > size) break;
  }

  lua_pop(L,1);

  if (size != (int)lua_objlen(L,-1)) {
    programError("Array size mismatch in call to polygon");
    return 0;
  }
  polyBufferX = (Sint16*)malloc(size * sizeof(Sint16));
  lua_pushnil(L);
  i=0;
  while(lua_next(L,-2) != 0) {
    polyBufferX[i++] = (Sint16)lua_tonumber(L,-1);
    lua_pop(L,1);
    if (i > size) break;
  }

  drawPolygon(pi64.fb, polyBufferX, polyBufferY, size, pi64.r, pi64.g, pi64.b, pi64.alpha, pi64.filled);

  free(polyBufferX);
  free(polyBufferY);
  return 0;
}

int putpixelBinding(lua_State *L) {
    Uint32 pixel;
    Uint8 r, g, b, a;
    int x, y;

    a = luaL_optnumber(L, 6, pi64.alpha);
    b = luaL_optnumber(L, 5, pi64.b);
    g = luaL_optnumber(L, 4, pi64.g);
    r = luaL_optnumber(L, 3, pi64.r);
    // y = pi64.fb->height - 1 - lua_tonumber(L,2);
    y = lua_tonumber(L,2);
    x = lua_tonumber(L, 1);
    
    setPixelWithAlpha(pi64.fb, x, y, r, g, b, a);
    
    return 0;
}

int getpixelBinding(lua_State *L) {
    Uint32 pixel;
    Uint8 r, g, b;
    int x, y;

    x = lua_tonumber(L,-2);
    // y = pi64.fb->height - 1 - lua_tonumber(L,-1);
    y = lua_tonumber(L,-1);

    SDL_LockSurface(pi64.fb->screen);
    if (x < 0 || x >= pi64.fb->width || y < 0 || y >= pi64.fb->height) {
        pixel = 0;
    } else {
        int bpp;
        unsigned char *p;

        bpp = pi64.fb->screen->format->BytesPerPixel;
        p = ((unsigned char*) pi64.fb->screen->pixels)+
                             (y*pi64.fb->screen->pitch)+(x*bpp);
        switch(bpp) {
        case 1: pixel = *p; break;
        case 2: pixel = *(Uint16 *)p; break;
        case 3:
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
            pixel = p[0]|p[1]<<8|p[2]<<16;
#else
            pixel = p[2]|p[1]<<8|p[0]<<16;
#endif
        case 4: pixel = *(Uint32*)p; break;
        default: return 0; break;
        }
    }
    SDL_GetRGB(pixel,pi64.fb->screen->format,&r,&g,&b);
    SDL_UnlockSurface(pi64.fb->screen);
    /* Return the pixel as three values: r, g, b. */
    lua_pushnumber(L,r);
    lua_pushnumber(L,g);
    lua_pushnumber(L,b);
    return 3;
}

int spriteBinding(lua_State *L) {
    char filename[1024];
    int x, y, angle, antialiasing;
    double zoom;
    sprite *sprite;
    strcpy(filename,lua_tostring(L, 1));
    x = luaL_optnumber(L, 2, -1);
    y = luaL_optnumber(L, 3, -1);
    angle = luaL_optnumber(L,4,0);
    antialiasing = lua_toboolean(L,5);
    zoom = luaL_optnumber(L,6,1);
    sprite = spriteLoad(L,filename);
    if (x >= 0 && y >= 0)
      spriteBlit(pi64.fb, sprite, x, y, -1, angle, antialiasing, zoom);
    return 1;
}

int arcBinding(lua_State *L) {
    int x,y,r,start,end;
    x = lua_tonumber(L,-5);
    y = lua_tonumber(L,-4);
    r = lua_tonumber(L,-3);
    start = lua_tonumber(L,-2);
    end = lua_tonumber(L,-1);
    drawArc(pi64.fb,x,y,r,start,end,pi64.r,pi64.g,pi64.b,pi64.alpha);
    return 0;
}

int rotateBinding(lua_State *L) {
    float x,y,cx,cy,rx,ry;
    x = lua_tonumber(L,-5);
    y = lua_tonumber(L,-4);
    cx = lua_tonumber(L,-3);
    cy = lua_tonumber(L,-2);    
    float radians = lua_tonumber(L,-1);
    rx = x;
    ry = y;
    x -= cx;
    y -= cy;
    rx = x * cos(radians) - y * sin(radians);
    ry = x * sin(radians) + y * cos(radians);
    rx += cx;
    ry += cy;  
    lua_pushnumber(L,rx);
    lua_pushnumber(L,ry);
    return 2;
}

int elapsedtimer(lua_State *L) {
    lua_pushnumber(L,mstime());
    return 1;
}

int videoInit(lua_State *L) {
    int n = lua_gettop(L);
    if (n==4) {
       int w = lua_tointeger(L, 1);
       int h = lua_tointeger(L, 2);
       int bpp = lua_tointeger(L, 3);
       int full = lua_tointeger(L, 4);

       if (bpp != 8 && bpp != 16 && bpp != 24 && bpp != 32 && bpp != 0) {
          lua_pushnil(L);
          return 1;
       }
       if ( w < 0 ||  h < 0 ) {
          lua_pushnil(L);
          return 1;
       }
       if ( pi64.fb != NULL) {
            SDL_FreeSurface(pi64.fb->screen);
            free(pi64.fb);
       }
           
       pi64.fb = createFrameBuffer(w,h,bpp,full);
       pi64.width=w;
       pi64.height=h;
       resetProgram();
       POWEROFF_BUTTON_X = (pi64.fb->width-18);
       POWEROFF_BUTTON_Y = 18;
       // SAVE_BUTTON_X = (pi64.fb->width-BORDER_R-13);
       SAVE_BUTTON_X = (pi64.fb->width-18);
       SAVE_BUTTON_Y = (pi64.fb->height-16);        
       csizeEditor(INTP, pi64.fb);                    
       csizeEditor(E, pi64.fb);
       lua_pushinteger(L,1);
       return 1;
    } 
    else
        lua_pushnil(L);
    return 1;
}

int setBorder(lua_State *L) {
    int n = lua_gettop(L);
    if (n==4) {
       BORDER_T = lua_tointeger(L, 1);
       BORDER_B = lua_tointeger(L, 2);
       BORDER_L = lua_tointeger(L, 3);
       BORDER_R = lua_tointeger(L, 4);
       INTP->margin_top = BORDER_T;
       INTP->margin_bottom = BORDER_B;
       INTP->margin_left = BORDER_L;
       INTP->margin_right = BORDER_R;
       INTP->screencols = (pi64.fb->width-INTP->margin_left-INTP->margin_right) / fontKerning;
       INTP->screenrows = (pi64.fb->height-INTP->margin_top-INTP->margin_bottom) / fontHeight;
       E->margin_top = BORDER_T;
       E->margin_bottom = BORDER_B;
       E->margin_left = BORDER_L;
       E->margin_right = BORDER_R;
       E->screencols = (pi64.fb->width-E->margin_left-E->margin_right) / fontKerning;
       E->screenrows = (pi64.fb->height-E->margin_top-E->margin_bottom) / fontHeight;
    }
    return 0;
}

int setfontchar(lua_State *L) {
    int cno, w, h;
    unsigned char *bfont; 
    short c; 
        
    cno = lua_tointeger(L, 1);
    w = fontWidth;
    h = fontHeight;
    
    lua_pushnil(L);
    int i=0;
    if (UBitmapFont[cno]!=NULL)
       free(UBitmapFont[cno]);
    bfont = malloc(w*h/8+1);
    while (lua_next(L,-2) != 0) {
        c = lua_tointeger(L,-1);
        bfont[i++] = c;        
        lua_pop(L,1);
        if (i > w*h/8) break;
    }        
    UBitmapFont[cno] = bfont;
    return 0;
}

int sethlcolor(lua_State *L) { 
    checkArgNull(L);
    int idx = lua_tointeger(L,1); 
    int r = lua_tointeger(L,2); 
    int g = lua_tointeger(L,3); 
    int b = lua_tointeger(L,4); 
    int a = luaL_optnumber(L,5,255); 
     
    syscolor hl;
    
    if (idx<HL_COLORS+1 && idx>0) {
        hl.r = r;
        hl.g = g;
        hl.b = b;
        hl.a = a;
        hlscheme[idx-1]=hl;
    }
    
    return 0;
}

int setsyscolor(lua_State *L) { 
    checkArgNull(L);
    int idx = lua_tointeger(L,1); 
    int r = lua_tointeger(L,2); 
    int g = lua_tointeger(L,3); 
    int b = lua_tointeger(L,4); 
    int a = luaL_optnumber(L,5,255); 
     
    syscolor hl;
    hl.r = r;
    hl.g = g;
    hl.b = b;
    hl.a = a;
    
    if (idx<SYS_COLORS+1 && idx>0) {
        hl.r = r;
        hl.g = g;
        hl.b = b;
        hl.a = a;
        sysscheme[idx-1]=hl;
    }
    
    return 0;
}

void SDLBindings(lua_State *L) {
    lua_pushcfunction(L,fillBinding);
    lua_setglobal(L,"fill");
    lua_pushcfunction(L,filledBinding);
    lua_setglobal(L,"filled");
    lua_pushcfunction(L,rectBinding);
    lua_setglobal(L,"rect");
    lua_pushcfunction(L,ellipseBinding);
    lua_setglobal(L,"ellipse");
    lua_pushcfunction(L,backgroundBinding);
    lua_setglobal(L,"background");
    lua_pushcfunction(L,triangleBinding);
    lua_setglobal(L,"triangle");
    lua_pushcfunction(L,lineBinding);
    lua_setglobal(L,"line");
    lua_pushcfunction(L,textBinding);
    lua_setglobal(L,"text");
    lua_pushcfunction(L,setFPSBinding);
    lua_setglobal(L,"setFPS");
    lua_pushcfunction(L,getpixelBinding);
    lua_setglobal(L,"getpixel");
    lua_pushcfunction(L,putpixelBinding);
    lua_setglobal(L,"putpixel");
    lua_pushcfunction(L,spriteBinding);
    lua_setglobal(L,"sprite");
    lua_pushcfunction(L,polygonBinding);
    lua_setglobal(L,"polygon");     
    lua_pushcfunction(L,arcBinding);
    lua_setglobal(L,"arc");     
    lua_pushcfunction(L,rotateBinding);
    lua_setglobal(L,"rotate");     
        
    lua_pushcfunction(L,elapsedtimer);
    lua_setglobal(L,"timer");  
    
    lua_pushcfunction(L,videoInit);
    lua_setglobal(L,"videoinit"); 
    
    lua_pushcfunction(L,setBorder);
    lua_setglobal(L,"setborder");     
    
    lua_pushcfunction(L,setfontchar);
    lua_setglobal(L,"setfont");  
    
    lua_pushcfunction(L,sethlcolor);
    lua_setglobal(L,"sethlcolor");   
    
    lua_pushcfunction(L,setsyscolor);
    lua_setglobal(L,"setsyscolor");      

/*    
    lua_pushcfunction(L,set_engine_fontchar);
    lua_setglobal(L,"setbasicfont");      
*/
        
}

