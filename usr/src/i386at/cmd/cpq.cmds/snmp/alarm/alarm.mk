#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cpq.cmds:snmp/alarm/alarm.mk	1.4"

include $(CMDRULES)

SNMP    = ..
PARSE   = ../smux/parse

LDLIBS   = -lsmux -lsnmp -lsnmpio -L $(PARSE) -lparse \
	   -L $(SNMP)/lib -lmon -lsocket -lnsl 

LOCALINC = -I$(SNMP)/define -I$(SNMP)/lib -I$(SNMP)/smux -I.
LOCALDEF = -Dfprintf=agentlog_fprintf -DCOMPAQ

PROBEFILE = agent.c
TARGET = alarm_agent
BINARIES = alarm_agent
MAKEFILE = alarm.mk

SRCS    = agent.c alarm.c alarm.h common.c event.h mibvar.c misc.c \
	  misc.h

OBJS    = agent.o alarm.o common.o mibvar.o misc.o 

all:
	@if [ -f $(PROBEFILE) ]; then \
		find $(BINARIES) \( ! -type f -o -links +1 \) \
		-exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
		$(MAKE) -f $(MAKEFILE) alarm_agent $(MAKEARGS) \
		"LOCALDEF=$(LOCALDEF)" ;\
	else \
		for fl in $(BINARIES); do \
			if [ ! -r $$fl ]; then \
				echo "ERROR: $$fl is missing" 1>&2 ;\
				false ;\
				break ;\
			fi \
		done \
	fi

alarm_agent: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS) $(LOCALINC)

clobber:  clean
	@if [ -f $(PROBEFILE) ]; then \
		$(RM) -f $(TARGET); \
	fi

clean:
	$(RM) -f $(OBJS) 

lintit:
	$(LINT) $(LOCALDEF) $(LOCALINC) $(LINTFLAGS) -I. *.c

install: all        
	[ -d $(USRBIN)/compaq/smux/alarm ] || mkdir -p $(USRBIN)/compaq/smux/alarm
	$(INS) -f $(USRBIN)/compaq/smux/alarm -m 0700 -u root -g other alarm_agent
