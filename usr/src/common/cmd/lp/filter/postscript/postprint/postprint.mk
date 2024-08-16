#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)lp:filter/postscript/postprint/postprint.mk	1.2.6.3"
#ident "$Header: postprint.mk 1.2 91/04/12 $"
#
# makefile for the ASCII file to PostScript translator.
#

include $(CMDRULES)

MAKEFILE=postprint.mk
ARGS=all

#
# Common header and source files have been moved to $(COMMONDIR).
#

COMMONDIR=../common
HOSTDIR =	$(ROOT)/$(MACH)/usr/share/lib/hostfontdir

#
# postprint doesn't use floating point arithmetic, so the -f flag isn't needed.
#

LOCALDEF= -DSYSV
LOCALINC= -I$(COMMONDIR)

CFILES=postprint.c \
       $(COMMONDIR)/request.c \
       $(COMMONDIR)/glob.c \
       $(COMMONDIR)/misc.c

HFILES=postprint.h \
       $(COMMONDIR)/comments.h \
       $(COMMONDIR)/ext.h \
       $(COMMONDIR)/gen.h \
       $(COMMONDIR)/path.h

POSTPRINT=postprint.o\
       $(COMMONDIR)/request.o\
       $(COMMONDIR)/glob.o\
       $(COMMONDIR)/misc.o

ALLFILES=README $(MAKEFILE) $(HFILES) $(CFILES)


all : postprint

install : postprint
	@if [ ! -d "$(BINDIR)" ]; then \
	    mkdir $(BINDIR); \
	    $(CH)chmod 775 $(BINDIR); \
	    $(CH)chgrp $(GROUP) $(BINDIR); \
	    $(CH)chown $(OWNER) $(BINDIR); \
	fi
	$(INS) -m 775 -u $(OWNER) -g $(GROUP) -f $(BINDIR) postprint
	@if [ ! -d "$(HOSTDIR)" ]; then \
	    mkdir $(HOSTDIR); \
	    $(CH)chmod 775 $(HOSTDIR); \
	    $(CH)chgrp $(GROUP) $(HOSTDIR); \
	    $(CH)chown $(OWNER) $(HOSTDIR); \
	fi
	$(INS) -m 664 -u $(OWNER) -g $(GROUP) -f $(HOSTDIR) encodemap 
#	cp postprint $(BINDIR)
#	chmod 775 $(BINDIR)/postprint
#	chgrp $(GROUP) $(BINDIR)/postprint
#	chown $(OWNER) $(BINDIR)/postprint

postprint : $(POSTPRINT)
	$(CC) -o postprint $(POSTPRINT) $(LDFLAGS) $(SHLIBS)

$(COMMONDIR)/glob.o : $(COMMONDIR)/glob.c $(COMMONDIR)/gen.h
	cd $(COMMONDIR); $(CC) $(CFLAGS) $(DEFLIST) -c glob.c

$(COMMONDIR)/misc.o : $(COMMONDIR)/misc.c $(COMMONDIR)/ext.h $(COMMONDIR)/gen.h
	cd $(COMMONDIR); $(CC) $(CFLAGS) $(DEFLIST) -c misc.c

$(COMMONDIR)/request.o : $(COMMONDIR)/request.c $(COMMONDIR)/ext.h $(COMMONDIR)/gen.h $(COMMONDIR)/path.h
	cd $(COMMONDIR); $(CC) $(CFLAGS) $(DEFLIST) -c request.c

postprint.o : $(HFILES)

clean :
	rm -f $(POSTPRINT)

clobber : clean
	rm -f postprint

list :
	pr -n $(ALLFILES) | $(LIST)

lintit:
