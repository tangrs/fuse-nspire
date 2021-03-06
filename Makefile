CC	?= gcc

CFLAGS	:= -O3 -Wall
CFLAGS 	+= $(shell pkg-config libnspire --cflags)

CFLAGS_OSXFUSE := -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26
CFLAGS_OSXFUSE += -I/usr/local/include/osxfuse/fuse

LIBS	:= -lfuse_ino64
LIBS	+= -framework CoreServices -framework IOKit
LIBS	+= $(shell pkg-config libnspire --libs)

TARGETS = nspire.o dir.o file.o stat.o

%.o: %.c
	$(CC) -c $(CFLAGS_OSXFUSE) $(CFLAGS) -o $@ $<

all: nspire

nspire: $(TARGETS)
	$(CC) -o $@ $(LIBS) $^

clean:
	rm -f $(TARGETS) *.o
