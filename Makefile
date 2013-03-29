CC	:= gcc

CFLAGS	:= -O3 -Wall
CFLAGS 	+= $(shell pkg-config libnspire --cflags)

CFLAGS_OSXFUSE := -D__FreeBSD__=10 -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26
CFLAGS_OSXFUSE += -I/usr/local/include/osxfuse/fuse

LIBS	:= -lfuse
LIBS	+= -framework CoreServices -framework IOKit
LIBS	+= $(shell pkg-config libnspire --libs)

TARGETS = nspire.o

%.o: %.c
	$(CC) -c $(CFLAGS_OSXFUSE) $(CFLAGS) -o $@ $<

all: nspire

nspire: $(TARGETS)
	$(CC) -o $@ $(LIBS) $^

clean:
	rm -f $(TARGETS) *.o
