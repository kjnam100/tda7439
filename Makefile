CC=gcc
CFLAGS=-Wall
OBJECTS=tda7439.o
BINARIES=tda7439
LDFLAGS+= -lm -lwiringPi

all : $(BINARIES)

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $< $(LDFLAGS)

clean:
	rm -f $(OBJECTS) $(BINARIES)

FORCE:
.PHONY: FORCE
