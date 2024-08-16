#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)libnsl:common/lib/libnsl/rexec/rexec.mk	1.1.5.3"
#ident "$Header: $"

include $(LIBRULES)
include ../libnsl.rules

RINCLUDES=	$(INC)/rx.h

.SUFFIXES:	.c .o 

LOCALDEF=-DNO_IMPORT $(NSL_LOCALDEF)
LOCALINC=-I.
SOURCES = rxlib.c rx_mt.c
OBJECTS = rxlib.o rx_mt.o
LIBOBJECTS = ../rxlib.o
INCLUDES=	$(INC)/sys/byteorder.h \
		$(INC)/signal.h \
		$(INC)/errno.h \
		$(INC)/termio.h \
		$(INC)/stdio.h \
		$(INC)/stropts.h \
		$(INC)/rx.h \
		rx_mt.h

all:	$(OBJECTS)
	cp $(OBJECTS) ../


rxlib.o:	rxlib.c \
		$(INC)/sys/byteorder.h \
		$(INC)/signal.h \
		$(INC)/errno.h \
		$(INC)/termio.h \
		$(INC)/stdio.h \
		$(INC)/stropts.h \
		$(INC)/rx.h \
		rx_mt.h

rx_mt.o:	rx_mt.c \
		$(INC)/errno.h \
		$(INC)/rx.h \
		rx_mt.h 

lintit:
	$(LINT) $(LINTFLAGS) $(INCLIST) $(DEFLIST) $(SOURCES)

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(LIBOBJECTS)

size: all
	$(SIZE) $(LIBOBJECTS)

strip: all
	$(STRIP) $(LIBOBJECTS)
