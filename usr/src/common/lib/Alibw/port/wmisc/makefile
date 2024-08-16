#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)libw:port/wmisc/makefile	1.2.2.3"
#ident "$Header: makefile 1.4 91/06/27 $"

#
# makefile for libw/port/wmisc
#
#

include $(LIBRULES)

.SUFFIXES: .p
PROF=
NONPROF=
INCLIBW=../../inc
LOCALINC=-I$(INCLIBW)

OBJECTS =  getwidth.o mbftowc.o

SOURCES =  getwidth.c mbftowc.c

POBJECTS =  getwidth.p mbftowc.p

nonprof: $(OBJECTS)

prof: $(POBJECTS)

getwidth.o getwidth.p:	 $(INC)/ctype.h \
		 $(INCLIBW)/libw.h 
mbftowc.o mbftowc.p:	 $(INC)/ctype.h \
		 $(INC)/stdlib.h 
.c.o .c.p:
	$(NONPROF)@echo $*.c:
	$(NONPROF)$(CC) $(DEFLIST) $(CFLAGS) $(L16DEFS) -c $*.c
	$(PROF)@echo $*.c:
	$(PROF)$(CC) $(DEFLIST) $(CFLAGS) $(L16DEFS) -c -p $*.c && mv $(*F).o $*.p
