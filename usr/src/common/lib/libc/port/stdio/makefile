#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libc-port:stdio/makefile	1.14.7.16"
#
# makefile for libc/port/stdio
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
_filbuf.o      _findbuf.o     _findiop.o     _flsbuf.o      _inwc.o	\
_outwc.o       _unwc.o        clrerr.o       ctermid.o      cuserid.o	\
data.o         fclose.o       fdopen.o       feof.o         ferror.o	\
fgets.o        fgetwc.o       fgetws.o       fileno.o       flockfile.o	\
flush.o        fopen.o        fpos.o         fputc.o        fputs.o	\
fputwc.o       fputws.o       fread.o        fseek.o        ftell.o	\
funflush.o     funlockfile.o  fwrite.o       getchar.o      getpass.o	\
gets.o         getw.o         getwchar.o     popen.o        pushpop.o	\
putchar.o      puts.o         putw.o         putwchar.o     rew.o	\
setbuf.o       setvbuf.o      stdiostrs.o    system.o       tempnam.o	\
tmpfile.o      tmpnam.o       ungetc.o       ungetwc.o	    bufsync.o

POBJECTS=$(OBJECTS:.o=.p)
PIC_OBJECTS=$(OBJECTS:.o=.P)

nonprof: $(OBJECTS)

prof: $(POBJECTS)

pic: $(PIC_OBJECTS)

$(OBJECTS):	$(INCPORT)/stdiom.h

.c.o .c.p .c.P:
	$(NONPROF)@echo $*.c:
	$(NONPROF)$(CC) $(INCLIST) $(DEFLIST) $(SDEFLIST) $(CFLAGS) $(LOCALFLAGS) -c $*.c
	$(PROF)@echo $*.c:
	$(PROF)$(CC) $(INCLIST) $(DEFLIST) $(CFLAGS) $(LOCALFLAGS) -c -p $*.c && mv $(*F).o $*.p
	$(PIC)@echo $*.c:
	$(PIC)$(CC) $(INCLIST) $(DEFLIST) $(CFLAGS) $(LOCALFLAGS) -c -DPIC -DDSHLIB -Kpic $*.c && mv $(*F).o $*.P
