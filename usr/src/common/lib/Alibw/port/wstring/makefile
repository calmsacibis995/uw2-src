#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)libw:port/wstring/makefile	1.2.2.3"
#ident "$Header: makefile 1.4 91/06/27 $"

#
# makefile for libw/port/wstring
#
#

include $(LIBRULES)

.SUFFIXES: .p
PROF=
NONPROF=
INCLIBW= ../../inc
LOCALINC=-I$(INCLIBW)

OBJECTS=\
strtows.o   wsntostr.o  wssize.o    wstostr.o   wsntostr.o	libwfcns.o

POBJECTS=\
strtows.p   wsntostr.p  wssize.p    wstostr.p   wsntostr.p	libwfcns.p

nonprof: $(OBJECTS)

prof: $(POBJECTS)

.c.o .c.p:
	$(NONPROF)@echo $*.c:
	$(NONPROF)$(CC) $(DEFLIST) $(CFLAGS) $(L16DEFS) -c $*.c
	$(PROF)@echo $*.c:
	$(PROF)$(CC) $(DEFLIST) $(CFLAGS) $(L16DEFS) -c -p $*.c && mv $(*F).o $*.p
