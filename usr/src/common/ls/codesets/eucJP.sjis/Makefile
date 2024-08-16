#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)langsup:common/ls/codesets/eucJP.sjis/Makefile	1.1"

TARGET=$(PKGDIR)/usr/lib/iconv
OBJS=_iconv.o _iconv_open.o _iconv_close.o
CFLAGS=-K PIC -c #-I$(ROOT)/usr/src/$(WORK)/lib/libc/port/inc

all: eucJP.sjis.so

install: all $(TARGET)
	cp eucJP.sjis.so $(TARGET)

eucJP.sjis.so: $(OBJS)
	$(CC) $(LDFLAGS) -G -o eucJP.sjis.so $(OBJS) -ldl

_iconv.o _iconv_open.o _iconv_close.o: _iconv_defs.h

clean:
	rm -f *.o

clobber: clean
	rm -f eucJP.sjis.so
