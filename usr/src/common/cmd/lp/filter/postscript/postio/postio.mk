#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)lp:filter/postscript/postio/postio.mk	1.6.7.3"
#ident "$Header: postio.mk 1.3 91/04/16 $"
#
# makefile for the RS-232 serial interface program for PostScript printers.
#

include $(CMDRULES)

MAKEFILE=postio.mk
ARGS=all

#
# Common source and header files have been moved to $(COMMONDIR).
#

COMMONDIR=../common

#
# postio doesn't use floating point arithmetic, so the -f flag isn't needed.
#

LOCALDEF= -DSYSV
LOCALINC= -I$(COMMONDIR)

SYSTEM=SYSV

CFILES=postio.c ifdef.c slowsend.c

HFILES=postio.h\
       ifdef.h\
       $(COMMONDIR)/gen.h

POSTIO=postio.o ifdef.o slowsend.o

ALLFILES=README $(MAKEFILE) $(HFILES) $(CFILES)


all : postio romfonts.ps

install : all 
	@if [ ! -d "$(BINDIR)" ]; then \
	    mkdir $(BINDIR); \
	    $(CH)chmod 775 $(BINDIR); \
	    $(CH)chgrp $(GROUP) $(BINDIR); \
	    $(CH)chown $(OWNER) $(BINDIR); \
	fi
	$(INS) -m 775 -u $(OWNER) -g $(GROUP) -f $(BINDIR) postio
	$(INS) -m 775 -u $(OWNER) -g $(GROUP) -f $(BINDIR) romfonts.ps
#	cp postio $(BINDIR)
#	$(CH)chmod 775 $(BINDIR)/postio
#	$(CH)chgrp $(GROUP) $(BINDIR)/postio
#	$(CH)chown $(OWNER) $(BINDIR)/postio

postio : $(POSTIO)
	@if [ "$(SYSTEM)" = "SYSV" -a -d "$(DKHOSTDIR)" ]; then \
	    EXTRA="-Wl,-L$(DKHOSTDIR)/lib -ldk"; \
	fi; \
	if [ "$(SYSTEM)" = "V9" ]; then \
	    EXTRA="-lipc"; \
	fi; \
	echo "	$(CC) -o postio $(POSTIO) $(LDFLAGS) $$EXTRA $(SHLIBS)"; \
	$(CC) -o postio $(POSTIO) $(LDFLAGS) $$EXTRA $(SHLIBS)

postio.o : $(HFILES)
slowsend.o : postio.h $(COMMONDIR)/gen.h
ifdef.o : ifdef.h $(COMMONDIR)/gen.h

clean :
	rm -f $(POSTIO)

clobber : clean
	rm -f postio

list :
	pr -n $(ALLFILES) | $(LIST)

lintit:

