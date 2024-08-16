#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/metamail/richmail/Makefile	1.1.4.4"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

# @(#)Makefile	1.5
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

VERS=2.3.6

CONFIGDIR = ..
LOCALINC= -I. -I${CONFIGDIR} -I../..
LOCALDEF=
CFLAGS = -O $(LOCALINC) $(LOCALDEF)
LDLIBS =
LDFLAGS=

include $(CMDRULES)

.SUFFIXES: .ln
.c.ln:
	$(LINT) $(LINTFLAGS) $(DEFLIST) -c $*.c > $*.lerr
	
RICHOBJS=richtext.o richlex.o richset.o usascii.o iso2022.o

LINTFILES = $(RICHOBJS:.o=.ln)

all:  richtext

richtext: $(RICHOBJS)
	$(CC) ${CFLAGS} -o richtext richtext.o $(RICHOBJS) -lmail -lcurses $(LDFLAGS) $(LDLIBS)

richtext.ln richtext.o:	richlex.h richset.h
richlex.ln richlex.o:	richlex.h richset.h
richset.ln richset.o:	richlex.h richset.h
usascii.ln usascii.o:	richlex.h richset.h
iso822.ln iso2022.o:	richlex.h richset.h

clean:
	-rm -f richtext *.o *.BAK

lintit: richtext.lint

richtext.lint: $(LINTFILES)
	$(LINT) $(LINTFLAGS) $(LINTFILES) > $@

tar:
	tar cvf richtext-$(VERS).tar *.c *.h richtext.1 Makefile COMMANDS NOTES
