#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)front_panel:panel.mk	1.9"

include $(CMDRULES)

MAKEFILE = panel.mk

MAINS = astdisplay astgraph panel astmonitor astcache

LDLIBS = -lgen -lx
INITD = $(ETC)/init.d
STOPPANEL = $(ETC)/rc0.d/K99ast
STARTPANEL1 = $(ETC)/rc1.d/S99ast
STARTPANEL2 = $(ETC)/rc2.d/S99ast

all: astdisplay astgraph panel astmonitor astcache

panel: panel.o
	$(CC) -O -o $@ $@.o $(LDLIBS)

astdisplay: astdisplay.o
	$(CC) -O -o $@ $@.o $(LDLIBS)

astgraph: astgraph.o
	$(CC) -O -o $@ $@.o $(LDLIBS)

astmonitor: astmonitor.o
	$(CC) -O -o $@ $@.o $(LDLIBS)

astcache: astcache.o
	$(CC) -O -o $@ $@.o $(LDLIBS)

install:	all
	$(INS) -f $(SBIN) -m 0755 -u bin -g bin panel
	$(INS) -f $(SBIN) -m 0755 -u bin -g bin astdisplay
	$(INS) -f $(SBIN) -m 0755 -u bin -g bin astgraph
	$(INS) -f $(SBIN) -m 0755 -u bin -g bin astmonitor
	$(INS) -f $(SBIN) -m 0755 -u bin -g bin astcache
	$(INS) -f $(INITD) -m 0444 -u root -g sys ast
	$(INS) -f $(ETC)/default -m 644 panel.cfg
	@rm -f $(STARTPANEL) $(STOPPANEL)
	-ln $(INITD)/ast $(STARTPANEL1)
	-ln $(INITD)/ast $(STARTPANEL2)
	-ln $(INITD)/ast $(STOPPANEL)

clean:
	rm -f *.o 

clobber: clean
	rm -f $(MAINS)

lintit:
	$(LINT) $(IFLAGS) $(LINTFLAGS) astdisplay.c
	$(LINT) $(IFLAGS) $(LINTFLAGS) astgraph.c
	$(LINT) $(IFLAGS) $(LINTFLAGS) astmonitor.c
	$(LINT) $(IFLAGS) $(LINTFLAGS) astcache.c
	$(LINT) $(IFLAGS) $(LINTFLAGS) panel.c

