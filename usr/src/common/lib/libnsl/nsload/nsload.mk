#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libnsl:common/lib/libnsl/nsload/nsload.mk	1.1"
#ident  "$Header: $"

 
include $(LIBRULES)
include ../libnsl.rules

.SUFFIXES:	.c .o 

LOCALDEF=	$(NSL_LOCALDEF)
LIBOBJECTS=	../nsload.o
OBJECTS=	nsload.o
SRCS=		nsload.c

all:		$(OBJECTS)
		if [ x$(CCSTYPE) != xCOFF ] ; \
		then \
			cp $(OBJECTS) ../ ; \
		fi

lintit: 
		$(LINT) $(LINTFLAGS) $(INCLIST) $(DEFLIST) $(SRCS)

clean:
		rm -f $(OBJECTS)

clobber:	clean
		rm -f $(LIBOBJECTS)

size:		all
		$(SIZE) $(LIBOBJECTS)

strip:		all
		$(STRIP) $(LIBOBJECTS)
