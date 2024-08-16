#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/nw/ipx/drvload/drvload.mk	1.3"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"$Id: drvload.mk,v 1.2 1994/02/18 15:16:19 vtag Exp $"

include $(UTSRULES)

include ../../local.defs

MAKEFILE=	drvload.mk
KBASE = ../../../..
LINTDIR = $(KBASE)/lintdir
DIR = net/nw/ipx/drvload

LFILE = $(LINTDIR)/ipx.ln

CFILES = lddrv.c
FILES = lddrv.o
LFILES = lddrv.ln

SRCFILES = $(CFILES)


all: $(FILES) 

install: all

clean:
	-rm -f *.o $(LFILES) *.L $(FILES)

clobber: clean

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

include $(MAKEFILE).dep
