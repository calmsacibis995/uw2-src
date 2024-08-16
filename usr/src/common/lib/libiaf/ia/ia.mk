#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)libiaf:common/lib/libiaf/ia/ia.mk	1.2.2.4"
#ident "$Header: ia.mk 1.3 91/03/14 $"

include $(LIBRULES)

# 
# Identification and Authentication Library: ia_* routines
#

LOCALDEF=-DNO_IMPORT $(PICFLAG)

INCLUDES=	\
		$(INC)/fcntl.h \
		$(INC)/ia.h \
		$(INC)/memory.h \
		$(INC)/search.h \
		$(INC)/sys/mman.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/types.h

LIBOBJS = getia_info.o
NSOBJS = nsgetia_info.o

OBJS =	../getia_info.o ../nsgetia_info.o

all:       $(INCLUDES) $(LIBOBJS) $(NSOBJS)

install:

clean:
	-rm -f *.o

clobber:	clean
	-rm -f $(OBJS)

nsgetia_info.c : getia_info.c
	[ -f $@ ] || ln -s $? $@

nsgetia_info.o: nsgetia_info.c $(INC)/ns.h
	$(CC) $(CFLAGS) -DNS $(DEFLIST) -c $*.c && cp $(*F).o ..

.c.o:
	$(CC) $(CFLAGS) $(DEFLIST) -c $*.c && cp $(*F).o ..

