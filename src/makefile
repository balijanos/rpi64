
SRCS=${wildcard rpi/*.c} ${wildcard gfx/*.c} 
OBJS:=${SRCS:.c=.o}
PKGS=sdl SDL_gfx SDL_image
CFLAGS=-O2 -Wall -std=c99 -Iinclude -Iinclude/lua51 -Irpi -Iinclude/gfx -linclude `pkg-config --cflags $(PKGS)`

# -Ispl -Ispl/constraints -Iinclude/chipmunk -Iinclude/chipmunk/constraints 
# CFLAGS+=-linclude `pkg-config --cflags $(PKGS)`
LDLIBS=-llua5.1 -lm `pkg-config --libs $(PKGS)`

all: pi64 

pi64: pi64.o rpi/input.o rpi/bitlib.o rpi/bitlib.o rpi/commonEditor.o rpi/editor.o rpi/framebuffer.o  rpi/interpreter.o rpi/intplua.o rpi/sdlua.o\
      gfx/SDL_framerate.o gfx/SDL_gfxBlitFunc.o gfx/SDL_gfxPrimitives.o gfx/SDL_imageFilter.o gfx/SDL_rotozoom.o 
      
clean:
	rm -f pi64 *.o rpi/*.o gfx/*.o

distclean: clean
	-($(MAKE) clean)
 
dep:
	$(CC) $(CFLAGS) $(LDLIBS) -MM *.c
