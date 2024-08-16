#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/nw/spx/test/tli/tli.mk	1.4"
#ident	"$Id: tli.mk,v 1.2 1994/01/29 00:59:08 vtag Exp $"

include $(UTSRULES)

include ../../../local.defs 
include ./test.defs

MAKEFILE=	tli.mk
KBASE = ../../../../..
LINTDIR = $(KBASE)/lintdir
DIR = net/nw/spx/test/tli
LDLIBS = -lnsl 

LFILE = $(LINTDIR)/tli.ln

CFILES = microsleep.c \
	accept.c alloc.c bind.c close.c connect.c control.c error.c \
	event.c free.c getinfo.c getstate.c listen.c look.c maxconn.c open.c \
	optmgmt.c rcvconn.c sndrcv.c sndrcvdi.c sndrcvre.c \
	sndrcvud.c tliclnt.c tlifunc.c tliprcol.c tlisrvr.c tlisolo.c \
	unbind.c nwu.c

FILES = microsleep.o \
	accept.o alloc.o bind.o close.o connect.o control.o error.o \
	event.o free.o getinfo.o getstate.o listen.o look.o maxconn.o open.o \
	optmgmt.o rcvconn.o sndrcv.o sndrcvdi.o sndrcvre.o \
	sndrcvud.o tliclnt.o tlifunc.o tliprcol.o tlisrvr.o tlisolo.o \
	unbind.o nwu.o

LFILES = microsleep.ln \
	accept.ln alloc.ln bind.ln close.ln connect.ln control.ln error.ln \
	event.ln free.ln getinfo.ln getstate.ln listen.ln look.ln maxconn.ln open.ln \
	optmgmt.ln rcvconn.ln sndrcv.ln sndrcvdi.ln sndrcvre.ln \
	sndrcvud.ln tliclnt.ln tlifunc.ln tliprcol.ln tlisrvr.ln tlisolo.ln \
	unbind.ln nwu.ln

SRCFILES = $(CFILES)

TCLSR_OBJS = accept.o connect.o control.o event.o listen.o look.o maxconn.o \
	microsleep.o optmgmt.o rcvconn.o sndrcv.o sndrcvdi.o sndrcvre.o \
	sndrcvud.o tlifunc.o tliprcol.o unbind.o nwu.o

TCLNT_OBJS = tliclnt.o $(TCLSR_OBJS)

TSRVR_OBJS = tlisrvr.o $(TCLSR_OBJS)

TSOLO_OBJS =tlisolo.o alloc.o bind.o close.o control.o error.o free.o \
	getinfo.o getstate.o microsleep.o open.o tlifunc.o tliprcol.o nwu.o

TSOLO = tsolo.o $(TSOLO_OBJS)

all: $(FILES)  tclient tserver tsolo

tclient: $(TCLNT_OBJS)
	$(CC) -o $@ $(LDLIBS) $(TCLNT_OBJS) 

tserver: $(TSRVR_OBJS)
	$(CC) -o $@ $(LDLIBS) $(TSRVR_OBJS)

tsolo: $(TSOLO_OBJS)
	$(CC) -o $@ $(LDLIBS) $(TSOLO_OBJS)

install: all

clean:
	-rm -f *.o $(LFILES) *.L $(FILES)

clobber: clean
	-rm -f tclient tserver tsolo

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
