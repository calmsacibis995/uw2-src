#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libnsl:common/lib/libnsl/netselect/netselect.mk	1.9.11.3"
#ident	"$Header: $"

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#	PROPRIETARY NOTICE (Combined)
#
# This source code is unpublished proprietary information
# constituting, or derived under license from AT&T's UNIX(r) System V.
# In addition, portions of such source code were derived from Berkeley
# 4.3 BSD under license from the Regents of the University of
# California.
#
#
#
#	Copyright Notice 
#
# Notice of copyright on this source code product does not indicate 
#  publication.
#
#	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#          All rights reserved.
# 

include $(LIBRULES)
include ../libnsl.rules

.SUFFIXES:	.c .o 

LOCALDEF=	$(NSL_LOCALDEF)
INCLUDES=	netcspace.h \
	 	netsel_mt.h \
	 	$(INC)/netconfig.h \
	 	$(INC)/sys/netconfig.h 
OBJECTS=	netselect.o netsel_mt.o
LIBOBJECTS=	../netselect.o ../netsel_mt.o
SRCS=		$(OBJECTS:.o=.c)

all:		$(OBJECTS)
		cp $(OBJECTS) ../

netselect.o:	netselect.c \
		netcspace.h \
		netsel_mt.h \
		$(INC)/netconfig.h \
		$(INC)/sys/types.h \
		$(INC)/stdio.h \
		$(INC)/string.h \
		$(INC)/ctype.h

netsel_mt.o:	netsel_mt.c \
		netsel_mt.h \
		$(INC)/stdlib.h \
		$(INC)/thread.h \
		$(INC)/synch.h

lintit:
		lint $(LINTFLAGS) $(INCLIST) $(DEFLIST) $(SRCS)

clean:
		rm -f $(OBJECTS)

clobber:	clean
		rm -f $(LIBOBJECTS)

size:		all
		$(SIZE) $(LIBOBJECTS)

strip:		all
		$(STRIP) $(LIBOBJECTS)
