#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/pathrouter/pathrouter.mk	1.1"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident "@(#)makefile	1.5 'attmail mail(1) command'"
# @(#)Makefile	1.0 (pathrouter) 25/11/92

# Makefile for smail (not a installation makefile)

INS= install
USRBIN= /usr/bin
USRLIB= /usr/lib
ETC= /etc

# VERS equals version of the OS. one of SVR3|| SVR4 || SVR4_1
VERS	=	-DSVR4

# defaults for SVR4 and later
LOCALINC=	-I..
LOCALDEF=	$(VERS)
CFLAGS	=	-O $(LOCALINC) $(LOCALDEF)
LDFLAGS =
LIBS	=	-lmail

include $(CMDRULES)

OBJECTS = main.o map.o resolve.o misc.o headers.o getpath.o blook.o
LNFILES=${OBJECTS:.o=.ln}

.SUFFIXES: .c .ln

.c.ln:
	lint -y -c $(CFLAGS) $<

all:	pathrouter

lintit:	llib-lrouter.ln

llib-lrouter.ln:	$(LNFILES)
	lint -oxxx $(LNFILES)

pathrouter:		$(OBJECTS)	
		$(CC) -o pathrouter $(OBJECTS) $(LIBS) $(LDFLAGS)

clean:
		rm -f *.o *.ln a.out core $(LNFILES)

clobber:	clean
		rm -f pathrouter 

install:	pathrouter
	$(INS) -f $(USRLIB)/mail/surrcmd -m 511 -u bin -g bin pathrouter

strip:
	strip pathrouter

localinstall:	pathrouter
	$(INS) -f /usr/lib/mail/surrcmd -m 511 -u bin -g bin pathrouter

