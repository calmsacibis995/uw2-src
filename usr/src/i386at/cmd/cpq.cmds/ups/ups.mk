#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cpq.cmds:ups/ups.mk	1.4"

include $(CMDRULES)

SNMP = ../snmp

LDLIBS   = $(SNMP)/lib/libmon.a -lsocket -lnsl 

LOCALINC = -I$(SNMP)/define -I$(SNMP)/lib -I$(SNMP)/smux -I.
LOCALDEF = -Dfprintf=agentlog_fprintf -DCOMPAQ

TARGET = cpqupsd

SRCS    = ups.c fileio.c y.tab.c lex.yy.c agent.c common.c

OBJS    = ups.o fileio.o y.tab.o lex.yy.o agent.o common.o

MAKEFILE = ups.mk
PROBEFILE = ups.c 
BINARIES = cpqupsd

all:
	> ups.log
	@if [ -f $(PROBEFILE) ]; then \
		find $(BINARIES) \( ! -type f -o -links +1 \) \
		    -exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
		$(MAKE) -f $(MAKEFILE) binaries $(MAKEARGS) ;\
	else \
		for fl in $(BINARIES) cpqups.etc ups.cfg ; do \
			if [ ! -f $$fl ]; then \
				echo "ERROR: $$fl is missing" 1>&2 ;\
				false ;\
				break ;\
			fi \
		done \
	fi


install: all
	[ -d $(USRBIN)/compaq/ups ] || mkdir -p $(USRBIN)/compaq/ups
	[ -d $(ROOT)/$(MACH)/var/spool/compaq/ups ] || mkdir -p $(ROOT)/$(MACH)/var/spool/compaq/ups
	$(INS) -f $(USRBIN)/compaq/ups -m 0700 -u root -g other cpqupsd
	$(INS) -f $(ROOT)/$(MACH)/var/spool/compaq/ups -m 0644 -u root -g other ups.cfg
	$(INS) -f $(USRBIN)/compaq/ups -m 0644 -u root -g other cpqups.etc
	$(INS) -f $(ROOT)/$(MACH)/var/spool/compaq/ups -m 0644 -u root -g other ups.log
	

binaries: cpqupsd

cpqupsd: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)

y.tab.c: parse.y
	$(YACC) -d parse.y

lex.yy.c: y.tab.h lex.l
	$(LEX) lex.l

y.tab.h: y.tab.c


clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

clean:
	$(RM) -f $(OBJS) y.tab.c y.tab.h lex.yy.c

lintit: y.tab.c lex.yy.c
	$(LINT) $(LOCALDEF) $(LOCALINC) $(LINTFLAGS) -I. *.c
