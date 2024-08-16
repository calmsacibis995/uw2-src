#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cpq.cmds:snmp/smux/parse/parse.mk	1.2"

include $(LIBRULES)

LOCALDEF        =  -Dfprintf=agentlog_fprintf
LOCALINC        =  -I../../define -I../../lib -I..

OWN=bin
GRP=bin

SOURCES = cfg.c reg.c parse.c scan.c
OBJECTS = cfg.o reg.o parse.o scan.o

LIBRARY = libparse.a
MAKEFILE = parse.mk
BINARIES = $(LIBRARY) 
PROBEFILE = scan.c

all:
	@if [ -f $(PROBEFILE) ]; then \
		find $(BINARIES) \( ! -type f -o -links +1 \) \
		    -exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
		$(MAKE) -f $(MAKEFILE) binaries $(MAKEARGS) ;\
	else \
		for fl in config registry.mib; do \
			if [ ! -f $$fl ]; then \
				echo "ERROR: $$fl is missing" 1>&2 ;\
				false ;\
				break ;\
			fi \
		done \
	fi

binaries: $(LIBRARY)

$(LIBRARY):    $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS)

install: all
	-[ -d $(USRBIN)/compaq/smux ] || mkdir -p $(USRBIN)/compaq/smux
	$(INS) -f $(USRBIN)/compaq/smux -m 0700 -u root -g other registry.mib
	$(INS) -f $(USRBIN)/compaq/smux -m 0700 -u root -g other config

clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

clean:
	$(RM) -f *.o core

lintit:

