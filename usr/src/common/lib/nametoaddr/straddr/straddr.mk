#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nametoaddr:common/lib/nametoaddr/straddr/straddr.mk	1.1.10.1"
#ident	"$Header: $"

#Makefile for straddr.so

include $(LIBRULES)

LIBSODIR=	/usr/lib/
LIBNAME=	straddr.so
OBJECTS=	straddr.o
SRCS=           $(OBJECTS:.o=.c)
LOCALDEF=	-D_REENTRANT $(PICFLAG) -DPIC 
LOCALLDFLAGS=	-dy -G -ztext -h $(LIBSODIR)$(LIBNAME)

all:		$(LIBNAME)

$(LIBNAME):	$(OBJECTS)
		$(LD) $(LOCALLDFLAGS) -o $(LIBNAME) $(OBJECTS) -l nsl

straddr.o:	$(INC)/stdio.h $(INC)/xti.h $(INC)/netdir.h \
			$(INC)/netconfig.h $(INC)/ctype.h straddr.c

clean:
		rm -f $(OBJECTS)

clobber:	clean
		rm -f $(LIBNAME)

install:	all
		$(INS) -f $(USRLIB) $(LIBNAME)

lintit:
