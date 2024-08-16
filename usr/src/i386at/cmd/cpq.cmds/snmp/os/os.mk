#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cpq.cmds:snmp/os/os.mk	1.3"

include $(CMDRULES)

SNMP = ..

LDLIBS   = $(SNMP)/lib/libmon.a -lsocket -lnsl -lgen -lc -Bstatic -lmas

LOCALINC = -I$(SNMP)/define -I$(SNMP)/lib -I$(SNMP)/smux -I.
LOCALDEF = -Dfprintf=agentlog_fprintf -DCOMPAQ

TARGET = os_agent

SRCS    = agent.c common.c cpu.c def.h fs.c info.c misc.c misc.h
OBJS    = agent.o common.o cpu.o fs.o info.o misc.o

PROBEFILE = agent.c
BINARIES = $(TARGET)
MAKEFILE = os.mk

all:
	@if [ -f $(PROBEFILE) ]; then \
		find $(BINARIES) \( ! -type f -o -links +1 \) \
		    -exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
		$(MAKE) -f $(MAKEFILE) binaries $(MAKEARGS) ;\
	else \
		for fl in $(BINARIES); do \
			if [ ! -f $$fl ]; then \
				echo "ERROR: $$fl is missing" 1>&2 ;\
				false ;\
				break ;\
			fi \
		done \
	fi

binaries: $(TARGET)

os_agent: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)

clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

clean:
	$(RM) -f $(OBJS) y.tab.c y.tab.h lex.yy.c

lintit:
	$(LINT) $(LOCALDEF) $(LOCALINC) $(LINTFLAGS) -I. *.c

install: all
	-[ -d $(USRBIN)/compaq/smux/os ] || mkdir -p $(USRBIN)/compaq/smux/os
	$(INS) -f $(USRBIN)/compaq/smux/os -m 0700 -u root -g other os_agent

