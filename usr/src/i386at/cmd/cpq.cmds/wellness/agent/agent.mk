#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cpq.cmds:wellness/agent/agent.mk	1.6"

include $(CMDRULES)

SNMP_BASE	= ../../snmp
LIB	= $(SNMP_BASE)/lib
DEFINE	= $(SNMP_BASE)/define
SMUX	= $(SNMP_BASE)/smux
CIPC	= $(SNMP_BASE)/cipc/src
LDLIBS	= $(LIB)/libmon.a -lelf

HDRS    = asr.h correctable.h def.h ftps.h misc.h thermal.h common.h \
	  critical.h eisa.h post.h $(LIB)/main.h $(LIB)/obj.h

LOCALINC = -I. -I../.. -I$(LIB) -I$(DEFINE) -I$(SMUX) -I$(CIPC) 

LOCALDEF = -Dfprintf=agentlog_fprintf -DCOMPAQ

TARGET = wellness_agent
BINARIES = wellness_agent
MAKEFILE = agent.mk
PROBEFILE = agent.c

SRCS	= agent.c misc.c post.c common.c critical.c ftps.c \
          correctable.c thermal.c asr.c eisa.c lib.c $(HDRS)

OBJS	= agent.o misc.o post.o common.o critical.o correctable.o \
          lib.o asr.o eisa.o thermal.o ftps.o

all:
	> wellness.log
	@if [ -f $(PROBEFILE) ]; then \
		find $(BINARIES) \( ! -type f -o -links +1 \) \
		-exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
		$(MAKE) -f $(MAKEFILE) $(TARGET) $(MAKEARGS) \
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

install: all
	[ -d $(USRBIN)/compaq/wellness ] || mkdir -p $(USRBIN)/compaq/wellness
	[ -d $(ROOT)/$(MACH)/var/spool/compaq/wellness ] || mkdir -p $(ROOT)/$(MACH)/var/spool/compaq/wellness
	$(INS) -f $(USRBIN)/compaq/wellness -m 0700 -u root -g other wellness_agent
	$(INS) -f $(USRBIN)/compaq/wellness -m 0644 -u root -g other cpqwell.etc
	$(INS) -f $(ROOT)/$(MACH)/var/spool/compaq/wellness -m 0600 -u root -g other wellness.log

wellness_agent: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS) $(LOCALINC)

clobber:  clean
	@if [ -f $(PROBEFILE) ]; then \
		$(RM) -f $(TARGET);\
	fi

clean:
	$(RM) -f $(OBJS) 

lintit:
	$(LINT) $(LOCALDEF) $(LOCALINC) $(LINTFLAGS) -I. *.c
