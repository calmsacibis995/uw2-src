#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cpq.cmds:snmp/stdeq/stdeq.mk	1.5"

include $(CMDRULES)

SNMP = ..

LDLIBS   =  -lelf $(SNMP)/lib/libmon.a -lsocket -lnsl -lgen 

LOCALINC = -I$(SNMP)/define -I$(SNMP)/lib -I$(SNMP)/smux -I.
LOCALDEF = -Dfprintf=agentlog_fprintf -DCOMPAQ
PROBEFILE = agent.c
BINARIES = $(TARGET)
MAKEFILE = stdeq.mk

TARGET = stdeq_agent

SRCS    = graphics.c agent.c common.c cpu.c  misc.c display.c eisa.c ev.c \
	  geteisa.c interp.c lib.c mem.c memodule.c

OBJS    = graphics.o display.o agent.o common.o cpu.o  misc.o eisa.o geteisa.o \
	  interp.o lib.o mem.o ev.o memodule.o

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

$(TARGET): $(OBJS)  
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)


clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

clean:
	$(RM) -f $(OBJS) inspect/*.o

lintit:
	$(LINT) $(LOCALDEF) $(LOCALINC) $(LINTFLAGS) -I. *.c

install: all
	-[ -d $(USRBIN)/compaq/smux/stdeq ] || mkdir -p $(USRBIN)/compaq/smux/stdeq
	$(INS) -f $(USRBIN)/compaq/smux/stdeq -m 0700 -u root -g other stdeq_agent
	$(INS) -f $(USRBIN)/compaq/smux/stdeq -m 0700 -u root -g other cpqbssa.txt

