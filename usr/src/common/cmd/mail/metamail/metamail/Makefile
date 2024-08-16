#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/metamail/metamail/Makefile	1.1.4.3"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

# @(#)Makefile	1.4
# Copyright (c) 1991 Bell Communications Research, Inc. (Bellcore)
# 
# Permission to use, copy, modify, and distribute this material 
# for any purpose and without fee is hereby granted, provided 
# that the above copyright notice and this permission notice 
# appear in all copies, and that the name of Bellcore not be 
# used in advertising or publicity pertaining to this 
# material without the specific, prior written permission 
# of an authorized representative of Bellcore.  BELLCORE 
# MAKES NO REPRESENTATIONS ABOUT THE ACCURACY OR SUITABILITY 
# OF THIS MATERIAL FOR ANY PURPOSE.  IT IS PROVIDED "AS IS", 
# WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES.
#

LOCALINC= -I. -I.. -I../..
LOCALDEF=
CFLAGS = -O $(LOCALINC) $(LOCALDEF)
LDLIBS =
LDFLAGS=

include $(CMDRULES)

.SUFFIXES: .ln
.c.ln:
	$(LINT) $(LINTFLAGS) $(DEFLIST) -c $*.c > $*.lerr
	
all: metamail mimencode mailto splitmail

codes.ln codes.o:  codes.c ../config.h

mailto.ln mailto.o: mailto.c ../config.h

metamail.ln metamail.o: metamail.c ../config.h

shared.ln shared.o:  shared.c ../config.h

splitmail.ln splitmail.o:  shared.c ../config.h

lintit: mimencode.lint mailto.lint splitmail.lint metamail.lint

mimencode: mmencode.o codes.o
	rm -f mmencode
	$(CC) $(CFLAGS) -o mimencode  mmencode.o  codes.o -lmail $(LDFLAGS) $(LDLIBS)

mimencode.lint: mmencode.ln codes.ln
	$(LINT) $(LINTFLAGS) mmencode.ln codes.ln > $@

mailto: mailto.o codes.o shared.o
	rm -f mailto
	$(CC) $(CFLAGS) -o mailto  mailto.o  codes.o shared.o -lmail -lcurses $(LDFLAGS) $(LDLIBS)

mailto.lint: mailto.ln codes.ln shared.ln
	$(LINT) $(LINTFLAGS) mailto.ln codes.ln shared.ln > $@

splitmail: splitmail.o shared.o
	rm -f splitmail
	$(CC) $(CFLAGS) -o splitmail  splitmail.o shared.o -lmail $(LDFLAGS) $(LDLIBS)

splitmail.lint: splitmail.ln shared.ln
	$(LINT) $(LINTFLAGS) splitmail.ln shared.ln > $@

metamail:  metamail.o codes.o uue.o shared.o
	rm -f metamail
	$(CC) $(CFLAGS) -o metamail  metamail.o  codes.o uue.o shared.o -lmail $(LDFLAGS) $(LDLIBS)

metamail.lint: metamail.ln codes.ln uue.ln shared.ln
	$(LINT) $(LINTFLAGS) metamail.ln codes.ln uue.ln shared.ln > $@

clobber: clean
	rm -f mimencode mimencode.lint mailto mailto.lint splitmail splitmail.lint metamail metamail.lint

clean:
	-rm -f *.o *.BAK *.ln *.lerr
