#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)lp:filter/postscript/postcomm/postcomm.mk	1.2.6.2"
#ident "$Header: postcomm.mk 1.2 91/04/12 $"

include $(CMDRULES)

#
# makefile for the program that sends files to PostScript printers.
#

MAKEFILE=postcomm.mk
ARGS=all

#
# Common source and header files have been moved to $(COMMONDIR).
#

COMMONDIR=../common

#
# postcomm doesn't use floating point arithmetic, so the -f flag isn't needed.
#

LOCALDEF= -DSYSV
LOCALINC= -I$(COMMONDIR)

#CFILES=postcomm.c ifdef.c
CFILES=postcomm.c

HFILES=postcomm.h

POSTIO=$(CFILES:.c=.o)

ALLFILES=README $(MAKEFILE) $(HFILES) $(CFILES)


all : postcomm

install : postcomm
	@if [ ! -d "$(BINDIR)" ]; then \
	    mkdir $(BINDIR); \
	    $(CH)chmod 775 $(BINDIR); \
	    $(CH)chgrp $(GROUP) $(BINDIR); \
	    $(CH)chown $(OWNER) $(BINDIR); \
	fi
	$(INS) -m 775 -u $(OWNER) -g $(GROUP) -f $(BINDIR) postcomm
#	cp postcomm $(BINDIR)
#	chmod 775 $(BINDIR)/postcomm
#	chgrp $(GROUP) $(BINDIR)/postcomm
#	chown $(OWNER) $(BINDIR)/postcomm

postcomm : $(POSTIO)
	if [ -d "$(DKHOSTDIR)" ]; \
	    then $(CC) -o postcomm $(POSTIO) $(LDFLAGS) -Wl,-L$(DKHOSTDIR)/lib -ldk $(SHLIBS); \
	    else $(CC) -o postcomm $(POSTIO) $(LDFLAGS) $(SHLIBS); \
	fi

postcomm.o : $(HFILES)

clean :
	rm -f *.o

clobber : clean
	rm -f postcomm

list :
	pr -n $(ALLFILES) | $(LIST)

lintit:

