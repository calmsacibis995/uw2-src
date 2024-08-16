#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)langsup:i386/ls/cmd/fcomp/Makefile	1.1"

include $(CMDRULES)

all: fcomp

fcomp: fcomp.o
	$(CC) $(CFLAGS) -o $@ $@.o $(LDFLAGS) $(LDLIBS)

fcomp.o: fcomp.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h

clean:
	-rm -f *.o

clobber: clean
	-rm -f fcomp

install: all
	$(INS) -f $(ROOT)/$(MACH)/usr/lbin -m 0555 -u bin -g bin fcomp

strip:	all
	$(STRIP) fcomp
