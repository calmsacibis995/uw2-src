#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)lp:filter/postscript/dpost/dpost.mk	1.2.6.2"
#ident "$Header: dpost.mk 1.2 91/04/12 $"
#
# makefile for the troff post-processor for PostScript printers.
#

include $(CMDRULES)

MAKEFILE=dpost.mk
ARGS=all

#
# Common header and source files have been moved to $(COMMONDIR).
#

COMMONDIR=../common

#
# There are potential conflicts on systems running DWB2.0. dpost is included in
# that package and installed in /usr/bin. If DWB_DPOST exists on this system and
# you're doing an install we'll replace it with the new program.
#

DWB_DPOST=/usr/bin/dpost

#
# dpost uses some floating point arithmetic, so if you're running on a system
# without floating point hardware add the -f option to the definition of CFLAGS.
#

LOCALDEF= -DSYSV
LOCALINC= -I$(COMMONDIR)
LDLIBS = -lm

CFILES=dpost.c draw.c color.c pictures.c ps_include.c\
       $(COMMONDIR)/glob.c \
       $(COMMONDIR)/misc.c \
       $(COMMONDIR)/request.c \
       $(COMMONDIR)/tempnam.c

HFILES=dpost.h ps_include.h \
       $(COMMONDIR)/comments.h \
       $(COMMONDIR)/dev.h \
       $(COMMONDIR)/ext.h \
       $(COMMONDIR)/gen.h \
       $(COMMONDIR)/path.h

DPOST=dpost.o draw.o color.o pictures.o ps_include.o\
       $(COMMONDIR)/glob.o\
       $(COMMONDIR)/misc.o\
       $(COMMONDIR)/request.o \
       $(COMMONDIR)/tempnam.o

ALLFILES=README $(MAKEFILE) $(HFILES) $(CFILES)


all : dpost

install : dpost
	@if [ ! -d "$(BINDIR)" ]; then \
	    mkdir $(BINDIR); \
	    $(CH)chmod 775 $(BINDIR); \
	    $(CH)chgrp $(GROUP) $(BINDIR); \
	    $(CH)chown $(OWNER) $(BINDIR); \
	fi
	$(INS) -m 775 -u $(OWNER) -g $(GROUP) -f $(BINDIR) dpost
#	cp dpost $(BINDIR)
#	chmod 775 $(BINDIR)/dpost
#	chgrp $(GROUP) $(BINDIR)/dpost
#	chown $(OWNER) $(BINDIR)/dpost
#	if [ -f "$(DWB_DPOST)" -a -w "$(DWB_DPOST)" -a -x "$(DWB_DPOST)" ]; then \
#	    cp dpost $(DWB_DPOST); \
#	fi

dpost : $(DPOST)
	$(CC) -o dpost $(DPOST) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

$(COMMONDIR)/glob.o : $(COMMONDIR)/glob.c $(COMMONDIR)/gen.h
	cd $(COMMONDIR); $(CC) $(CFLAGS) $(DEFLIST) -c glob.c

$(COMMONDIR)/misc.o : $(COMMONDIR)/misc.c $(COMMONDIR)/ext.h $(COMMONDIR)/gen.h
	cd $(COMMONDIR); $(CC) $(CFLAGS) $(DEFLIST) -c misc.c

$(COMMONDIR)/request.o : $(COMMONDIR)/request.c $(COMMONDIR)/ext.h $(COMMONDIR)/gen.h $(COMMONDIR)/path.h
	cd $(COMMONDIR); $(CC) $(CFLAGS) $(DEFLIST) -c request.c

$(COMMONDIR)/tempnam.o : $(COMMONDIR)/tempnam.c
	cd $(COMMONDIR); $(CC) $(CFLAGS) $(DEFLIST) -c tempnam.c

dpost.o : $(HFILES)
draw.o : $(COMMONDIR)/ext.h $(COMMONDIR)/gen.h
color.o : $(COMMONDIR)/ext.h $(COMMONDIR)/gen.h
pictures.o : $(COMMONDIR)/comments.h $(COMMONDIR)/gen.h

ps_include.o : ps_include.h
ps_include.h : ps_include.ps
	awk -f ps_include.awk ps_include.ps >ps_include.h

clean :
	rm -f $(DPOST)

clobber : clean
	rm -f dpost

list :
	pr -n $(ALLFILES) | $(LIST)

lintit:
