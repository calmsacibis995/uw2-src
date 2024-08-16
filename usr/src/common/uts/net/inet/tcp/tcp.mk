#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/inet/tcp/tcp.mk	1.9"
#ident	"$Header: $"

#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#

include $(UTSRULES)

MAKEFILE=	tcp.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = net/inet/tcp

TCP = tcp.cf/Driver.o
LFILE = $(LINTDIR)/tcp.ln

MODULES = $(TCP)

FILES = tcp_debug.o tcp_input.o tcp_main.o tcp_output.o tcp_state.o \
	tcp_subr.o tcp_timer.o

LFILES = tcp_debug.ln tcp_input.ln tcp_main.ln tcp_output.ln tcp_state.ln \
	 tcp_subr.ln tcp_timer.ln

CFILES = tcp_debug.c tcp_input.c tcp_main.c tcp_output.c tcp_state.c \
	   tcp_subr.c tcp_timer.c 
HFILES = tcp.h tcp_debug.h tcp_fsm.h tcp_seq.h tcp_timer.h tcp_var.h tcpip.h

SRCFILES = $(CFILES)

all:	$(MODULES)

install: all
	(cd tcp.cf; $(IDINSTALL) -R$(CONF) -M tcp)

$(TCP):	$(FILES)
	$(LD) -r -o $(TCP) $(FILES)

clean: 
	-rm -f $(FILES) $(LFILES) *.L *.klint $(TCP)

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e tcp

$(LINTDIR):
	-mkdir -p $@

tcp.klint: $(SRCFILES)

klintit: tcp.klint
	klint $(SRCFILES) >tcp.klint 2>&1 || true

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES);	\
	do \
		echo $$i; \
	done

netinetHeaders = \
	tcp.h \
	tcp_f.h \
	tcp_debug.h \
	tcp_var.h \
	tcp_fsm.h \
	tcp_seq.h \
	tcp_kern.h \
	tcp_timer.h \
	tcpip.h

headinstall: $(netinetHeaders)
	@-[ -d $(INC)/netinet ] || mkdir -p $(INC)/netinet
	@for f in $(netinetHeaders); \
	 do \
	    $(INS) -f $(INC)/netinet -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
