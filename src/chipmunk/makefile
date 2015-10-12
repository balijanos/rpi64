SRCS = ${wildcard *.c} ${wildcard constraints/*.c}

OBJS := ${SRCS:.c=.o}

CFLAGS += -Wall -std=c99 -DNDEBUG

program_INCLUDE_DIRS := ../include/chipmunk/constraints ../include/chipmunk constraints
program_LIBRARY_DIRS :=
program_LIBRARIES := 

CPPFLAGS += $(foreach includedir,$(program_INCLUDE_DIRS),-I$(includedir))
LDFLAGS += $(foreach librarydir,$(program_LIBRARY_DIRS),-L$(librarydir))
LDFLAGS += $(foreach library,$(program_LIBRARIES),-l$(library))

CC=cc

LINK.c := $(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)

.PHONY: all

../lib/chipmunk.a: $(OBJS)
	ar rcs $@ $^

all: ../lib/chipmunk.a

clean:
	rm -f ../lib/chipmunk.a *.o constraints/*.o 

distclean: clean
	-($(MAKE) clean)

