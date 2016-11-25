#################################################
#
# Makefile for SuperC - SNES code
#
#################################################

AS   = wla-65816

LINK = wlalink
LFLAGS = -b -d -v -S
ASFLAGS =


TARGET = snes.bin
LIBS =
OBJS = main.o interrupt.o spcdrv.o



ifdef MAKESMC
  ASFLAGS += -D _MAKESMC
  LFLAGS = -r -d -v -S
  TARGET := $(TARGET:.bin=.smc)
endif


.PHONY: all clean



all: $(TARGET) tags


$(TARGET): $(OBJS) $(LIBS) linkfile Makefile
	$(LINK) $(LFLAGS) linkfile $@

smc:
	make MAKESMC=1

.SUFFIXES: .o .lib .s .asm
.s.o:
	$(AS) -M $(ASFLAGS) -o $@ $< >$(@:.o=.d)
	$(AS) $(ASFLAGS) -o $@ $<

.asm.o:
	$(AS) -M $(ASFLAGS) -o $@ $< >$(@:.o=.d)
	$(AS) $(ASFLAGS) -o $@ $<

.s.lib:
	$(AS) -M $(ASFLAGS) -l $@ $< >$(@:.lib=.d)
	$(AS) $(ASFLAGS) -l $@ $<

.asm.lib:
	$(AS) -M $(ASFLAGS) -l $@ $< >$(@:.lib=.d)
	$(AS) $(ASFLAGS) -l $@ $<

-include *.d

linkfile: $(OBJS) $(LIBS)
	@echo -n  "Creating linkfile ... "
	@echo     "[libraries]" > linkfile
	@for lib in $(LIBS) ; do echo bank 3 slot 3 $$lib >> linkfile ; done
	@echo     "[objects]" >> linkfile
	@for obj in $(OBJS) ; do echo $$obj >> linkfile ; done
	@echo     "done!!"

# tagsファイルは、ctagsコマンドが無い場合は作成しません
tags: $(OBJS) $(LIBS) Makefile
	@if which ctags >/dev/null ; then \
	echo -n "Making tags file ... " ;\
	ctags -R ;\
	echo "done!!";\
	fi

clean:
	rm -rf $(TARGET:.spc=.bin) $(TARGET:.bin=.smc) $(TARGET:.bin=.sym) $(OBJS) $(LIBS) $(OBJS:.o=.d) $(LIBS:.lib=.d) linkfile

