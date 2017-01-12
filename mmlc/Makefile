############################################################
# Makefile for SuperC mml compiler
############################################################

### Compiler option
CLANG	= 1

### Platform option
#WIN32	= 1
#x86	= 1
#ARM	= 1

### Debug option
#DEBUG	= 1

############################################################

TARGET	= mmlc

ifdef CLANG
	CC_PRG= clang
	CXX_PRG= clang++
else
	CC_PRG= gcc
	CXX_PRG= g++
endif

ifdef WIN32
	BINPREFIX := .exe
	PLATFORM_ := windows
	CXXFLAGS += -DWIN32
	CFLAGS += -DWIN32
ifdef x86
	PREFIX := i686-w64-mingw32-
	CFLAGS += -Dx86
	LDFLAGS += -m32
else
	PREFIX := x86_64-w64-mingw32-
endif
else ifdef ARM
	PREFIX := arm-linux-gnueabihf-
endif

CC		:= $(PREFIX)$(CC_PRG)
CXX		:= $(PREFIX)$(CXX_PRG)

OBJCOPY		:= $(PREFIX)objcopy
ifdef x86
OBJFLAGS = -I binary -O elf32-i386 -B i386
else ifdef ARM
OBJFLAGS = -I binary -O elf32-littlearm -B arm
else
OBJFLAGS = -I binary -O elf64-x86-64 -B i386
endif


LDFLAGS	:= -lz

ifdef DEBUG
CFLAGS	+= -Wall -g -O0 -DDEBUG
else
CFLAGS	+= -Wall -O2
LDFLAGS	+= -s -Wl,--gc-sections -Wl,--discard-all
endif

ifdef WIN32
CFLAGS += -I../../zlib-1.2.8
LDFLAGS += -L../../zlib-1.2.8
endif

#LDFLAGS	= -Wl,--gc-sections -Wl,--no-define-common -Wl,--discard-all

OFILES	 = mmlc.o compile.o mmlman.o binaryman.o errorman.o spc.o timefunc.o pathfunc.o snsf.o address.o
OFILES	+= md5.o snes.o
PRECOMPILE_HEADER = gstdafx.h

.PHONY: all clean
all: $(TARGET)$(BINPREFIX)

$(TARGET)$(BINPREFIX): $(PRECOMPILE_HEADER).gch $(OFILES) Makefile
	$(CC) $(OFILES) $(LDFLAGS) -o $@


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
	rm -f $(TARGET)$(BINPREFIX) $(OFILES) $(OFILES:.o=.d) $(PRECOMPILE_HEADER).gch snes.bin

-include *.d


