#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)lp:filter/postscript/picpack/picpack.mk	1.2.6.2"
#ident "$Header: picpack.mk 1.2 91/04/12 $"
#
# makefile for the picture packing pre-processor.
#

include $(CMDRULES)

MAKEFILE=picpack.mk
ARGS=all

#
# Common header and source files have been moved to $(COMMONDIR).
#

COMMONDIR=../common

#
# picpack doesn't use floating point arithmetic, so the -f flag isn't needed.
#

LOCALDEF= -DSYSV
LOCALINC= -I$(COMMONDIR)

CFILES=picpack.c \
       $(COMMONDIR)/glob.c \
       $(COMMONDIR)/misc.c \
       $(COMMONDIR)/tempnam.c

HFILES=$(COMMONDIR)/ext.h \
       $(COMMONDIR)/gen.h \
       $(COMMONDIR)/path.h

PICPACK=picpack.o\
       $(COMMONDIR)/glob.o\
       $(COMMONDIR)/misc.o\
       $(COMMONDIR)/tempnam.o

ALLFILES=README $(MAKEFILE) $(HFILES) $(CFILES)


all : picpack

install : picpack
	@if [ ! -d "$(BINDIR)" ]; then \
	    mkdir $(BINDIR); \
	    $(CH)chmod 775 $(BINDIR); \
	    $(CH)chgrp $(GROUP) $(BINDIR); \
	    $(CH)chown $(OWNER) $(BINDIR); \
	fi
	$(INS) -m 775 -u $(OWNER) -g $(GROUP) -f $(BINDIR) picpack
#	cp picpack $(BINDIR)
#	chmod 775 $(BINDIR)/picpack
#	chgrp $(GROUP) $(BINDIR)/picpack
#	chown $(OWNER) $(BINDIR)/picpack

picpack : $(PICPACK)
	$(CC) -o picpack $(PICPACK) $(LDFLAGS) $(SHLIBS)

$(COMMONDIR)/glob.o : $(COMMONDIR)/glob.c $(COMMONDIR)/gen.h
	cd $(COMMONDIR); $(CC) $(CFLAGS) $(DEFLIST) -c glob.c

$(COMMONDIR)/misc.o : $(COMMONDIR)/misc.c $(COMMONDIR)/ext.h $(COMMONDIR)/gen.h
	cd $(COMMONDIR); $(CC) $(CFLAGS) $(DEFLIST) -c misc.c

$(COMMONDIR)/tempnam.o : $(COMMONDIR)/tempnam.c
	cd $(COMMONDIR); $(CC) $(CFLAGS) $(DEFLIST) -c tempnam.c

picpack.o : $(HFILES)

clean :
	rm -f $(PICPACK)

clobber : clean
	rm -f picpack

list :
	pr -n $(ALLFILES) | $(LIST)

lintit:

