#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)langsup:i386/ls/cmd/pcfont/Makefile	1.2"

include $(CMDRULES)

OBJECTS = pcfont.o loadfont.o

all: pcfont pcfont.dy

pcfont: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(NOSHLIBS)

pcfont.dy: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) 

pcfont.o:	pcfont.c \
		$(INC)/sys/types.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/errno.h \
		$(INC)/sys/at_ansi.h \
		$(INC)/sys/kd.h \
		$(INC)/sys/uio.h \
		$(INC)/stdio.h \
		$(INC)/ctype.h \
		$(INC)/string.h \
		$(INC)/stdlib.h \
		$(INC)/locale.h \
		$(INC)/unistd.h \
		$(INC)/fcntl.h \
		$(INC)/pfmt.h \
		pcfont.h

loadfont.o:	loadfont.c \
		$(INC)/sys/types.h \
		$(INC)/sys/at_ansi.h \
		$(INC)/sys/kd.h \
		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC)/ctype.h \
		$(INC)/string.h \
		$(INC)/stdlib.h \
		$(INC)/fcntl.h \
		pcfont.h

clean:
	-rm -f $(OBJECTS)

clobber: 	clean
	-rm -f pcfont pcfont.dy

install:	all
	$(INS) -f $(ROOT)/$(MACH)/sbin -m 555 -u bin -g bin pcfont
	$(INS) -f $(ROOT)/$(MACH)/usr/bin -m 555 -u bin -g bin pcfont.dy

strip:	all
	$(STRIP) pcfont
	$(STRIP) pcfont.dy
