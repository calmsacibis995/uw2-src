#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cpq.cmds:snmp/lib/lib.mk	1.2"

include $(LIBRULES)

MAKEFILE	= lib.mk

LOCALDEF	=  -Dfprintf=agentlog_fprintf  
LOCALINC	=  -I../define -I../smux

OWN=bin
GRP=bin

SOURCES = queue.c sem.c btree.c symtab.c lib.c shm.c lock.c \
	  log.c obj.c cipclib.c main.c 

OBJECTS = queue.o sem.o btree.o symtab.o lib.o shm.o lock.o \
	  log.o obj.o cipclib.o main.o

LIBRARY = libmon.a

PROBEFILE = queue.c
BINARIES = $(LIBRARY)

all:
	if [ -f $(PROBEFILE) ]; then \
		find $(BINARIES) \( ! -type f -o -links +1 \) \
		    -exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
		$(MAKE) -f $(MAKEFILE) binaries $(MAKEARGS) ;\
	fi

binaries:  $(BINARIES)

queue.o: queue.c 
	$(CC) $(CFLAGS) -DUSER_SPACE $(INCLIST) $(DEFLIST) -c $<

$(LIBRARY):    $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS)

install: all
	:

clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi
clean:
	$(RM) -f *.o core

lintit:
	$(LINT) -DUSER_SPACE $(LOCALDEF) $(LOCALINC) $(LINTFLAGS) -I. *.c 
