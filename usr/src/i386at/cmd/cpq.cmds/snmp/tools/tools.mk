#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cpq.cmds:snmp/tools/tools.mk	1.2"

include $(CMDRULES)

SNMP = ..

LDLIBS   =  -lelf libmon.a -lsocket -lnsl -lgen

LOCALINC = -I$(SNMP)/define -I$(SNMP)/lib -I$(SNMP)/smux -I.
LOCALDEF = -Dfprintf=agentlog_fprintf -DCOMPAQ

TARGET = browse

SRCS    = main.c

OBJS    = main.o

PROBEFILE = main.c
BINARIES = browse
MAKEFILE = tools.mk

all:
	@if [ -f $(PROBEFILE) ]; then \
		find $(BINARIES) \( ! -type f -o -links +1 \) \
		    -exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
		$(MAKE) -f $(MAKEFILE) binaries $(MAKEARGS) ;\
	fi

binaries:	$(TARGET)

libmon.a:  $(SNMP)/lib/libmon.a
	cp $(SNMP)/lib/libmon.a .
	ar d libmon.a main.o


$(TARGET): $(OBJS)   libmon.a
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)


clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

clean:
	$(RM) -f $(OBJS) libmon.a

lintit:
	$(LINT) $(LOCALDEF) $(LOCALINC) $(LINTFLAGS) -I. *.c

install:  all
	:

