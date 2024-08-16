#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/nw/spx/test/load/load.mk	1.2"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"$Id: load.mk,v 1.2 1994/04/25 20:37:02 meb Exp $"

include $(CMDRULES)

include ../../../local.defs 
include ./test.defs

MAKEFILE=	load.mk
KBASE = ../../../../..
LINTDIR = $(KBASE)/lintdir
DIR = net/nw/spx/test/load
LDLIBS = -lnsl 

LFILE = $(LINTDIR)/load.ln

CFILES = load.c load_d.c

FILES = load.o load_d.o

LFILES = load.ln load_d.ln

SRCFILES = $(CFILES)

all: $(FILES)  load load_d

load: load.o
	$(CC) -o $@ $(LDLIBS) load.o

load_d: load_d.o
	$(CC) -o $@ $(LDLIBS) load_d.o

install: all

clean:
	-rm -f *.o $(LFILES) *.L $(FILES)

clobber: clean
	-rm -f load load_d

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(LFILE):	$(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

FRC:

headinstall:

include $(UTSDEPEND)

#include $(MAKEFILE).dep
