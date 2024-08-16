#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libc-port:fmt/makefile	1.12.7.6"
#
# makefile for libc/port/fmt
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
_idoprnt.o    _idoscan.o    _iwdoprnt.o   _iwdoscan.o   _iwsdoprnt.o	\
_iwsdoscan.o  doprnt.o      doscan.o      fprintf.o     fscanf.o	\
fwprintf.o    fwscanf.o     parglist.o    printf.o      scanf.o		\
snprintf.o    sprintf.o     sscanf.o      swprintf.o    swscanf.o	\
vfprintf.o    vfscanf.o     vfwprintf.o   vfwscanf.o    vprintf.o	\
vscanf.o      vsnprintf.o   vsprintf.o    vsscanf.o     vswprintf.o	\
vswscanf.o    vwprintf.o    vwscanf.o     wparglist.o   wprintf.o	\
wscanf.o

POBJECTS=$(OBJECTS:.o=.p)

PIC_OBJECTS=$(OBJECTS:.o=.P)

nonprof: $(OBJECTS)

prof: $(POBJECTS)

pic: $(PIC_OBJECTS)

abi:
	# No min-ABI objects in this makefile

.c.o .c.p .c.P:
	$(NONPROF)@echo $*.c:
	$(NONPROF)$(CC) $(INCLIST) $(DEFLIST) $(SDEFLIST) $(CFLAGS) $(LOCALFLAGS) -c $*.c
	$(PROF)@echo $*.c:
	$(PROF)$(CC) $(INCLIST) $(DEFLIST) $(CFLAGS) $(LOCALFLAGS) -c -p $*.c && mv $(*F).o $*.p
	$(PIC)@echo $*.c:
	$(PIC)$(CC) $(INCLIST) $(DEFLIST) $(CFLAGS) $(LOCALFLAGS) -c -DDSHLIB -DPIC -Kpic $*.c && mv $(*F).o $*.P
