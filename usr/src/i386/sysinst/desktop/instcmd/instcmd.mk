#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/instcmd/instcmd.mk	1.1.1.15"

include	$(CMDRULES)

KB_OBJS = kb_remap.o kb_read_dk.o kb_read_kbd.o kb_read_font.o kb_misc.o 
OBJECTS = $(KB_OBJS) dkcomp.o lex.yy.o lex.yy.c

KB_SRC = $(ROOT)/$(MACH)/usr/lib/keyboard
DK_SRC = $(ROOT)/$(MACH)/usr/lib/mapchan
DK_KEYBOARDS = DE GB NL fr_CA DK IS NO fr_CH BE ES IT PT \
	de_CH CA FR IT2 SE es
FONT_INPUT = gr.8x14 gr.8x16 iso.8x14 iso.8x16

TARGET_MAINS = check_devs sflop kb_remap
HOST_MAINS = edsym sbfpack
LOCAL_MAINS = keycomp bmgr fcomp
MAINS = $(TARGET_MAINS) $(HOST_MAINS) $(LOCAL_MAINS)

KEYBOARDS = A01 DE GB NL US fr_CA AX DK IS NO fr_CH BE ES IT PT \
	de_CH CA FR IT2 SE es
DEAD_KEYS = 88591.dk
FONT = 88591

all: $(MAINS) $(KEYBOARDS) $(DEAD_KEYS) $(FONT)

check_devs: $$@.c \
		$(INC)/stdio.h \
		$(INC)/stdlib.h \
		$(INC)/unistd.h \
		$(INC)/errno.h \
		$(INC)/fcntl.h \
		$(INC)/sys/cram.h \
		$(INC)/sys/kd.h \
		$(INC)/sys/inline.h \
		$(INC)/sys/types.h \
		$(INC)/sys/fd.h \
		$(INC)/sys/ddi.h
	$(CC)  $(CFLAGS) -I$(INC) -o $@ $@.c $(LDFLAGS)

sflop: $$@.c \
		$(INC)/stdio.h \
		$(INC)/stdlib.h \
		$(INC)/unistd.h \
		$(INC)/errno.h \
		$(INC)/string.h \
		$(INC)/sys/types.h \
		$(INC)/fcntl.h \
		$(INC)/sys/vtoc.h \
		$(INC)/sys/fd.h
	$(CC)  $(CFLAGS) -I$(INC) -o $@ $@.c $(LDFLAGS)

kb_remap: $(KB_OBJS)
	$(CC)  $(CFLAGS) -I$(INC) -o $@ $(KB_OBJS) $(LDFLAGS)

kb_remap.o: $(@:.o=.c) \
	kb_remap.h \
	$(INC)/stdio.h \
	$(INC)/unistd.h \
	$(INC)/sys/kd.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/types.h \
	$(INC)/sys/fcntl.h

kb_read_dk.o: $(@:.o=.c) \
	kb_remap.h \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h

kb_read_font.o: $(@:.o=.c) \
	kb_remap.h \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/kd.h

kb_read_kbd.o: $(@:.o=.c) \
	kb_remap.h \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/kd.h

kb_misc.o: $(@:.o=.c) \
	kb_remap.h \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/kd.h

edsym: $$@.c \
		$(INC)/sys/types.h \
		$(INC)/sys/stat.h \
		$(INC)/stdio.h \
		$(INC)/fcntl.h \
		$(INC)/string.h
	$(HCC) $(CFLAGS) -I$(INC) -o $@ $@.c $(LDFLAGS)

sbfpack: $$@.c
	$(HCC) $(CFLAGS) -I$(INC) -o $@ $@.c $(LDFLAGS)

keycomp: $$@.c keycomp.h key_remap.h 
	$(HCC) $(CFLAGS) -I$(INC) -o $@ $@.c $(LDFLAGS)

dkcomp: $$@.o lex.yy.o
	$(HCC) $(CFLAGS) -I$(INC) -o $@ $@.o lex.yy.o $(LDFLAGS)

dkcomp.o: $(@:.o=.c) defs.h key_remap.h 
	$(HCC) $(CFLAGS) -I$(INC) -c $(@:.o=.c)

lex.yy.o: $(@:.o=.c) 
	$(HCC) $(CFLAGS) -I$(INC) -c $(@:.o=.c) 

lex.yy.c: lex.l defs.h
	$(LEX) $(LFLAGS) lex.l

bmgr: $$@.c \
		$(INC)/stdio.h \
		$(INC)/sys/types.h \
		$(INC)/sys/fcntl.h
	$(HCC) $(CFLAGS) -I$(INC) -o $@ $@.c $(LDFLAGS)

fcomp: $$@.c \
		fcomp.h \
		kb_remap.h \
		$(INC)/stdio.h \
		$(INC)/sys/types.h \
		$(INC)/sys/fcntl.h \
		$(INC)/sys/mman.h
	$(HCC) $(CFLAGS) -I$(INC) -o $@ $@.c $(LDFLAGS)

$(KEYBOARDS): keycomp $(KB_SRC)/$$@ 
	./keycomp $(KB_SRC)/$@ $@

$(DEAD_KEYS): dkcomp $(DK_SRC)/$$@
	./dkcomp $(DK_SRC)/$@ $@

$(FONT_INPUT):
	ln -s fontsrc/$@ $@

$(FONT): bmgr fcomp $(FONT_INPUT)
	./bmgr gr.8x14
	./bmgr gr.8x16
	mv gr.8x14.bm grbm.8x14
	mv gr.8x16.bm grbm.8x16
	./fcomp iso $(FONT)

install: all
	@for i in $(TARGET_MAINS) $(HOST_MAINS) ;\
	do \
		$(INS) -f $(PROTO)/bin $$i ;\
	done
	@for i in $(KEYBOARDS) ;\
	do \
		if [ ! -d $(PROTO)/desktop/keyboards/$$i ] ;\
		then \
			mkdir -p $(PROTO)/desktop/keyboards/$$i ;\
		fi ;\
		cp $$i kbmap ;\
		$(INS) -f $(PROTO)/desktop/keyboards/$$i kbmap ;\
	done
	@rm kbmap
	@cp $(DEAD_KEYS) dead_keys
	@for i in $(DK_KEYBOARDS) ;\
	do \
		$(INS) -f $(PROTO)/desktop/keyboards/$$i dead_keys ;\
	done
	@rm dead_keys
	@if [ ! -d $(PROTO)/desktop/keyboards/code_sets ] ;\
	then \
		mkdir -p $(PROTO)/desktop/keyboards/code_sets ;\
	fi
	@$(INS) -f $(PROTO)/desktop/keyboards/code_sets $(FONT)

clean:
	rm -f $(OBJECTS) $(FONT_INPUT) grbm.8x14 grbm.8x16

clobber: clean
	rm -f $(MAINS) $(KEYBOARDS) $(DEAD_KEYS) $(FONT)
