#--- generic settings ----------------------------------------------
# settings for C++ compiler:
C      = gcc
CC     = g++
CFLAGS = -O2 -Wall # -D_DEBUG -g
INCDIR = -Irtaudio -I../include/lua51

# linker settings:
LCC    = ar
LFLAGS = -rcs
LNAME = libproAudio.a
LIB    = $(LNAME)
LIBDIR =

LIBS = $(LIBDIR) $(LIB) -lpthread -lasound
LUALIB = -llua5.1  -ldl
CFLAGS += -DHAVE_GETTIMEOFDAY -D__LINUX_ALSA__ #-D__LINUX_OSS__
DLLFLAGS =  -fPIC -shared
DLLSUFFIX = .so
EXESUFFIX =
  
#--- make targets and rules ----------------------------------------

# by default, proteaAudio makes use of the included rtAudio backend
rtaudio: $(LNAME)

# the make all target additionally builds the Lua frontend and SDL backend, and therefore has additional dependencies
ALL = $(LNAME) proAudioRt$(DLLSUFFIX)
all: $(ALL)

# static library
OBJ = proAudio.o proAudioRt.o stb_vorbis.o rtaudio/RtAudio.o
$(LNAME) : $(OBJ)
	$(LCC) $(LFLAGS) $@ $^

# optional Lua frontend
lua: proAudioRt$(DLLSUFFIX)

proAudioRt$(DLLSUFFIX): proAudioRt_lua.o
	$(CC) -o audio.so $(DLLFLAGS) $^ $(LIBS) $(LUALIB)

# generic rules
.c.o:
	$(C) $(CFLAGS) $(INCDIR) -c $< -o $@
.cpp.o:
	$(CC) $(CFLAGS) $(INCDIR) -c $< -o $@
clean:
	rm -f *.o *~ $(OBJ) $(ALL)

#--- project specific dependencies ---------------------------------
HDR = proAudio.h proAudioRt.h
proAudioRt_lua.o:  proAudioRt_lua.cpp $(HDR)
proAudio.o: proAudio.cpp proAudio.h 
proAudioRt.o: proAudioRt.cpp $(HDR)
stb_vorbis.o: stb_vorbis.c
rtaudio/RtAudio.o: rtaudio/RtAudio.cpp rtaudio/RtAudio.h
