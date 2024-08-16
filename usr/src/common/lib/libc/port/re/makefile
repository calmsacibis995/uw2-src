#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libc-port:re/makefile	1.1"
#
# makefile for libc/port/re
#
#

include $(LIBRULES)

.SUFFIXES: .p .P
PROF=
NONPROF=
PIC=
ROOT=
INCPORT=../inc
INCCPU=../../$(CPU)/inc
SDEFLIST=
INCLIST=-I$(INCCPU) -I$(INCPORT)

OBJECTS=\
bracket.o   fnmatch.o   glob.o      regcomp.o   regdfa.o	\
regerror.o  regexec.o   regfree.o   regnfa.o    regparse.o

POBJECTS=$(OBJECTS:.o=.p)
PIC_OBJECTS=$(OBJECTS:.o=.P)

nonprof: $(OBJECTS)

prof: $(POBJECTS)

pic: $(PIC_OBJECTS)

.c.o .c.p .c.P:
	$(NONPROF)@echo $*.c:
	$(NONPROF)$(CC) $(INCLIST) $(DEFLIST) $(SDEFLIST) $(CFLAGS) $(LOCALFLAGS) -c $*.c
	$(PROF)@echo $*.c:
	$(PROF)$(CC) $(INCLIST) $(DEFLIST) $(CFLAGS) $(LOCALFLAGS) -c -p $*.c && mv $(*F).o $*.p
	$(PIC)@echo $*.c:
	$(PIC)$(CC) $(INCLIST) $(DEFLIST) $(CFLAGS) $(LOCALFLAGS) -c -DPIC -DDSHLIB -Kpic $*.c && mv $(*F).o $*.P
