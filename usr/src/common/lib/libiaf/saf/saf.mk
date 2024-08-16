#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libiaf:common/lib/libiaf/saf/saf.mk	1.1"
#ident "$Header: $"

include $(LIBRULES)

# 
# Service Access Facility Library: saf routines
#

LOCALDEF=-DNO_IMPORT $(PICFLAG)

INCLUDES=	\
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/fcntl.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/stropts.h \
	$(INC)/ctype.h \
	$(INC)/sys/conf.h \
	$(INC)/errno.h \
	$(INC)/signal.h \
	$(INC)/sac.h

LIBOBJS = doconfig.o \
	  checkver.o

OBJS =	../doconfig.o \
	../checkver.o

all:       $(INCLUDES) $(LIBOBJS)

install:

clean:
	-rm -f *.o

clobber:	clean
	-rm -f $(OBJS)

.c.o:
	$(CC) $(CFLAGS) $(DEFLIST) -c $*.c && cp $(*F).o ..

lintit:
	$(LINT) *.c
