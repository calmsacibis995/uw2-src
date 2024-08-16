#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cpq.cmds:eisautil/eisautil.mk	1.5"

include $(CMDRULES)

MAKEFILE=       eisautil.mk

BINS    = eisa_get eisa_nvm

PROBEFILE = eisa_nvm.c

all:
	@if [ -f $(PROBEFILE) ]; then \
		find $(BINS) \( ! -type f -o -links +1 \) \
			-exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
		$(MAKE) -f $(MAKEFILE) $(BINS) $(MAKEARGS) ;\
	else \
		if [ ! -r $(BINS) ]; then \
			echo "ERROR: $(BINS) is missing" 1>&2 ;\
			false ;\
			break ;\
		fi \
	fi

clean:
	rm -f *.o eisa_get eisa_nvm

clobber:	clean

eisa_get:	eisa_get.o
	$(CC) -O -s -o $@ eisa_get.o

eisa_nvm:	eisa_nvm.o
	$(CC) -O -s -o $@ eisa_nvm.o

eisa_get.o: eisa_get.c \
	$(INC)/sys/crom.h \
	$(INC)/sys/nvm.h

eisa_nvm.o: eisa_nvm.c \
	$(INC)/sys/crom.h \
	$(INC)/sys/nvm.h

install: all
	-[ -d $(USRBIN)/compaq ] || mkdir $(USRBIN)/compaq
	$(INS) -f $(USRBIN)/compaq -m 0555 -u bin -g bin eisa_get
	$(INS) -f $(USRBIN)/compaq -m 0555 -u bin -g bin eisa_nvm

