#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ipx-test:common/uts/net/nw/ipx/test/ipxecho/cmd/cmd.mk	1.1"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"$Id: cmd.mk,v 1.1 1994/03/02 15:43:25 meb Exp $"

include $(CMDRULES)

include ./local.defs 

MAKEFILE=	cmd.mk
KBASE = ../../../../../..
LINTDIR = $(KBASE)/lintdir
DIR = net/nw/net/nw/ipx/test/ipxecho/cmd
LDLIBS =  

LFILE = $(LINTDIR)/ipxecmd.ln

CFILES = ipxechod.c

FILES = ipxechod.o

LFILES = ipxechod.ln

SRCFILES = $(CFILES)

all: $(FILES)  ipxechod

ipxechod: $(FILES)
	$(CC) -o $@ $(LDLIBS) $(FILES) 

install: all
	$(INS) -f /tmp -m 0755 -u $(OWN) -g $(GRP) ipxechod

clean:
	-rm -f *.o $(LFILES) *.L $(FILES)
 
depend:

clobber: clean
	-rm -f ipxechod

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(LFILE):	$(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done
