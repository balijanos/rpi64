SRCS = ${wildcard *.c} ${wildcard constraints/*.c}

OBJS := ${SRCS:.c=.o}

CFLAGS += -Wall

program_INCLUDE_DIRS := constraints ../include/chipmunk ../chipmunk ../chipmunk/constraints\
			../include/chipmunk/constraints ../include/lua51
CPPFLAGS += $(foreach includedir,$(program_INCLUDE_DIRS),-I$(includedir))

DLLFLAGS =  -fPIC -shared
DLLSUFFIX = .so
LIBS = -llua5.1  -ldl ../lib/chipmunk.a

CC=cc

LINK.c := $(CC) $(CFLAGS) $(CPPFLAGS) $(LIBS)

.PHONY: all

all: $(OBJS)
	$(CC) -o spl.so $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(DLLFLAGS) $^ $(LIBS)
 
clean:
	rm -f spl.so *.o constraints/*.o 

distclean: clean
	-($(MAKE) clean)