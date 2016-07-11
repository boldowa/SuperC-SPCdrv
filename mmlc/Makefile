############################################################
# Makefile for SuperC mml compiler
############################################################

DEBUG	= 1
CLANG	= 1
#WIN32	= 1
#x86	= 1

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

LDFLAGS	:=

ifdef DEBUG
CFLAGS	= -Wall -g -O0 -DDEBUG
else
CFLAGS	= -Wall -O2 -s
LDFLAGS	+= -s
endif

ifdef WIN32
CFLAGS += -D_WIN32
LDFLAGS += -D_WIN32
ifdef x86
CFLAGS += -m32
LDFLAGS += -m32
endif
endif

#LDFLAGS	= -Wl,--gc-sections -Wl,--no-define-common -Wl,--discard-all

OFILES	 = mmlc.o compile.o mmlman.o binaryman.o errorman.o
OFILES	+= md5.o
PRECOMPILE_HEADER = gstdafx.h

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(PRECOMPILE_HEADER).gch $(OFILES) Makefile
	$(LD) $(LDFLAGS) $(OFILES) -o $@


.SUFFIXES: .c .o
.c.o:
	$(CC) -MMD -c $(CFLAGS) $<

$(PRECOMPILE_HEADER).gch: $(PRECOMPILE_HEADER)
	$(CC) $(CFLAGS) $<

clean:
	rm -f $(TARGET) $(OFILES) $(OFILES:.o=.d) $(PRECOMPILE_HEADER).gch

-include *.d


