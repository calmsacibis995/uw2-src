#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)libw:port/wctype/makefile	1.2.2.3"
#ident "$Header: makefile 1.4 91/06/27 $"

#
# makefile for libw/port/wctype
#
#

include $(LIBRULES)

.SUFFIXES: .p
PROF=
NONPROF=
INCLIBW= ../../inc
LOCALINC=-I$(INCLIBW)

OBJECTS=\
iswctype.o  trwctype.o loadtab.o

POBJECTS=\
iswctype.p  trwctype.p loadtab.p

nonprof: $(OBJECTS)

prof: $(POBJECTS)

.c.o .c.p:
	$(NONPROF)@echo $*.c:
	$(NONPROF)$(CC) $(DEFLIST) $(CFLAGS) $(L16DEFS) -c $*.c
	$(PROF)@echo $*.c:
	$(PROF)$(CC) $(DEFLIST) $(CFLAGS) $(L16DEFS) -c -p $*.c && mv $(*F).o $*.p

loadtab.o loadtab.p:			\
	$(INC)/locale.h $(INC)/fcntl.h $(INCLIBW)/wctype.h \
	$(INC)/sys/types.h $(INC)/sys/stat.h $(INCLIBW)/_locale.h
iswctype.o iswctype.p trwctype.o trwctype.p:		\
	$(INC)/ctype.h $(INCLIBW)/wctype.h
loadtab.o loadtab.p: loadtab.c
iswctype.o iswctype.p: iswctype.c
trwctype.o trwctype.p: trwctype.c
