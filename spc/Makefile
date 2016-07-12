#################################################
#
# Makefile for SuperC
#
#################################################

AS   = wla-spc700

LINK = wlalink
LFLAGS = -b -d -v -S
ASFLAGS =


TARGET = SuperC.bin
LIBS = libSuperC.lib
OBJS = main.o io.o music.o seqcmd.o se.o dsp.o seq_data.o brr.o
#OBJS = test.o io.o que.o music.o se.o



ifdef MAKESPC
  ASFLAGS += -D _MAKESPC
  LFLAGS = -r -d -v -S
  TARGET := $(TARGET:.bin=.spc)
endif


.PHONY: all clean



all: $(TARGET) tags


$(TARGET): $(OBJS) $(LIBS) linkfile Makefile
	$(LINK) $(LFLAGS) linkfile $@

spc:
	make MAKESPC=1

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
	@echo     "[objects]" > linkfile
	@for obj in $(OBJS) ; do echo $$obj >> linkfile ; done
	@echo     "[libraries]" >> linkfile
	@for lib in $(LIBS) ; do echo bank 3 slot 3 $$lib >> linkfile ; done
	@echo     "done!!"

# tagsファイルは、ctagsコマンドが無い場合は作成しません
tags: $(OBJS) $(LIBS) Makefile
	@if which ctags >/dev/null ; then \
	echo -n "Making tags file ... " ;\
	ctags -R ;\
	echo "done!!";\
	fi

clean:
	rm -rf $(TARGET:.spc=.bin) $(TARGET:.bin=.spc) $(TARGET:.bin=.sym) $(OBJS) $(LIBS) $(OBJS:.o=.d) $(LIBS:.lib=.d) linkfile

