#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sgs:libsgs/common/libsgs.mk	1.25"
#
#  makefile for libsgs.a
#
#
include $(CMDRULES)

.SUFFIXES: .p .P .A
HOSTCC=cc
NONPROF=
DEFLIST=-DNO_MSE
SDEFLIST=
INCLIST=-I. -I$(CPUINC)
HOSTAWK=awk
MAC=
HOSTAR=ar

OBJECTS=\
errlst.o	gettxt.o	mbstowcs.o	mbtowc.o	\
new_list.o	pfmt.o		qsort.o		setcat.o	\
setlabel.o	setlocale.o	strerror.o	strstr.o	\
strtoul.o	wcstombs.o	wctomb.o

.MUTEX:	setup ../libsgs.a
all:	setup ../libsgs.a

setup: _wchar.h qstr.h synonyms.h wcharm.h wctype.h stdlock.h

_wchar.h : ../../../../lib/libc/port/inc/_wchar.h
	cp $(?) $(@)

qstr.h : ../../../../lib/libc/port/inc/qstr.h
	cp $(?) $(@)

synonyms.h : ../../../../lib/libc/port/inc/synonyms.h
	cp $(?) $(@)

wcharm.h : ../../../../lib/libc/port/inc/wcharm.h
	cp $(?) $(@)

wctype.h : $(TINC)/wctype.h
	cp $(?) $(@)

stdlock.h : ../../../../lib/libc/port/inc/stdlock.h
	cp $(?) $(@)

.c.o .c.p .c.P .c.A:
	$(NONPROF)$(HOSTCC) $(DEFLIST) $(SDEFLIST) $(INCLIST) $(CFLAGS) -c $*.c

mbstowcs.c : ../../../../lib/libc/port/str/mbstowcs.c
	cp $(?) $(@)

mbtowc.c : ../../../../lib/libc/port/str/mbtowc.c
	cp $(?) $(@)

strstr.c : ../../../../lib/libc/port/str/strstr.c
	cp $(?) $(@)

strtoul.c : ../../../../lib/libc/port/str/strtoul.c
	cp $(?) $(@)

wcstombs.c : ../../../../lib/libc/port/str/wcstombs.c
	cp $(?) $(@)

wctomb.c : ../../../../lib/libc/port/str/wctomb.c
	cp $(?) $(@)

.MUTEX:	new_list.c errlst.c
new_list.c errlst.c : ../../../../lib/libc/port/gen/errlist \
                      ../../../../lib/libc/port/gen/errlist.awk
	$(HOSTAWK) -f ../../../../lib/libc/port/gen/errlist.awk <../../../../lib/libc/port/gen/errlist

../libsgs.a : $(OBJECTS)
	#
	# build the archive with the modules in correct order.
	$(HOSTAR) q ../libsgs.a `lorder $(OBJECTS) | $(TSORT)`

clean:
	rm -f *.o objlist lib.libsgs

clobber: clean
	rm -f uxsyserr errlst.c new_list.c ../libsgs.a _wchar.h	\
		mbstowcs.c mbtowc.c qstr.h synonyms.h strstr.c \
		strtoul.c wcharm.h wcstombs.c wctomb.c wctype.h stdlock.h
