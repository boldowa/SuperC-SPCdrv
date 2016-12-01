############################################################
# Makefile for SuperC mml compiler
############################################################

#DEBUG	= 1
CLANG	= 1
WIN32	= 1
x86	= 1

TARGET	= mmlc

ifdef CLANG
ifdef WIN32
TARGET	:= $(TARGET).exe
CC	:= i686-w64-mingw32-clang
LD	:= i686-w64-mingw32-clang
else
CC	:= clang
LD	:= clang
endif
else
ifdef WIN32
TARGET	:= $(TARGET).exe
CC	:= i686-w64-mingw32-gcc
LD	:= i686-w64-mingw32-gcc
else
CC	:= gcc
LD	:= gcc
endif
endif

LDFLAGS	:= -lz

ifdef DEBUG
CFLAGS	= -Wall -g -O0 -DDEBUG
else
CFLAGS	= -Wall -O2 -s
LDFLAGS	+= -s
endif

ifdef WIN32
CFLAGS += -D_WIN32 -I../../zlib-1.2.8
LDFLAGS += -D_WIN32 -L../../zlib-1.2.8
OBJCOPY	:= i686-w64-mingw32-objcopy
else
OBJCOPY	:= objcopy
endif

ifdef x86
CFLAGS += -m32
LDFLAGS += -m32
OBJFLAGS = -I binary -O elf32-i386 -B i386
else
OBJFLAGS = -I binary -O elf64-x86-64 -B i386
endif

#LDFLAGS	= -Wl,--gc-sections -Wl,--no-define-common -Wl,--discard-all

OFILES	 = mmlc.o compile.o mmlman.o binaryman.o errorman.o spc.o timefunc.o pathfunc.o snsf.o address.o
OFILES	+= md5.o snes.o
PRECOMPILE_HEADER = gstdafx.h

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(PRECOMPILE_HEADER).gch $(OFILES) Makefile
	$(LD) $(OFILES) $(LDFLAGS) -o $@


.SUFFIXES: .c .o
.c.o:
	$(CC) -MMD -c $(CFLAGS) $<

snes.o: snes.bin
	$(OBJCOPY) $(OBJFLAGS) $< $@

snes.bin: ../snes/snes.bin
	cp $< $@

../snes/snes.bin:
	make -C ../snes

$(PRECOMPILE_HEADER).gch: $(PRECOMPILE_HEADER)
	$(CC) $(CFLAGS) $<

clean:
	rm -f $(TARGET) $(OFILES) $(OFILES:.o=.d) $(PRECOMPILE_HEADER).gch snes.bin

-include *.d


