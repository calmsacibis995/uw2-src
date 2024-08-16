#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libc-port:str/makefile	1.10"
#
# makefile for libc/port/str
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
_mb2wc.o     _mbsize.o						\
_mf_str.o    _mf_wcs.o    _mfutil.o    _wssize.o    a64l.o	\
argvtostr.o  atof.o       atoi.o       atol.o       confstr.o	\
ecvt.o       gcvt.o       l64a.o       mbstowcs.o   mbtowc.o	\
memccpy.o    memchr.o     memcmp.o     memcpy.o     memmove.o	\
memset.o     nan.o        mbsrtowcs.o  strcat.o     strchr.o	\
strcmp.o     strcoll.o	  strcpy.o     strcspn.o    strdup.o    \
strerror.o   strlen.o     strlist.o    strncat.o    strncmp.o   \
strncpy.o    strpbrk.o    strrchr.o    strspn.o     strstr.o    \
strtoargv.o  strtod.o     strtof.o     strtok.o     strtok_r.o   strtol.o \
strtold.o    strtoul.o    strxfrm.o    wcsrtombs.o  wcscat.o	\
wcschr.o     wcscmp.o     wcscoll.o    wcscpy.o     wcscspn.o	\
wcslen.o     wcsncat.o    wcsncmp.o    wcsncpy.o    wcspbrk.o	\
wcsrchr.o    wcsspn.o     wcsstr.o     wcstod.o     wcstof.o	\
wcstok.o     wcstol.o     wcstold.o    wcstombs.o   wcstoul.o	\
wcsxfrm.o    wctomb.o     wctype.o     wcwidth.o    wcswidth.o	\
gettab.o     iconv.o      iconv_open.o ostrxfrm.o   wstok_gd.o

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
