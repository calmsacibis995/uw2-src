#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cpq.cmds:nic/nic.mk	1.3"
#ident	"$Header: $"

########################################################
# Copyright 1992, COMPAQ Computer Corporation
########################################################

include $(CMDRULES)

OWN	= root
GRP	= sys

SNMP	= ../snmp
LIB	= $(SNMP)/lib
LIBS	= $(LIB)/libmon.a

MAKEFILE = nic.mk
PROBEFILE = agent.c
BINARIES = $(TARGET)

LDLIBS   = $(SNMP)/lib/libmon.a -lsocket -lnsl -lgen 

LOCALINC = -I$(SNMP)/define -I$(SNMP)/lib -I$(SNMP)/smux -I. 
LOCALDEF = -Dfprintf=agentlog_fprintf -DCOMPAQ

HDRS	= def.h $(LIB)/main.h $(LIB)/obj.h
SRCS	= agent.c ifmap.c dot3.c dot5.c macs.c $(HDRS)
OBJS	= agent.o ifmap.o dot3.o dot5.o macs.o 
TARGET	= nic_agent

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

binaries: $(BINARIES)

nic_agent:	$(LIBS) $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)

$(LIB)/libmon.a: 
	cd $(LIB); make -f lib.mk

clobber:  clean
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

clean:
	$(RM) -f $(OBJS)

lintit:
	$(LINT) $(LOCALDEF) $(LOCALINC) $(LINTFLAGS) -I. *.c

install: all
	-[ -d $(USRBIN)/compaq/nic ] || mkdir -p $(USRBIN)/compaq/nic
	-[ -d $(ETC)/init.d ] || mkdir -p $(ETC)/init.d
	$(INS) -f $(USRBIN)/compaq/nic -m 0700 -u $(OWN) -g $(GRP) nic_agent
	$(INS) -f $(ETC)/init.d -m 0444 -u $(OWN) -g $(GRP) nicinit
	-ln $(ETC)/init.d/nicinit $(ETC)/rc2.d/S99nic
	-ln $(ETC)/init.d/nicinit $(ETC)/rc1.d/K99nic
	-ln $(ETC)/init.d/nicinit $(ETC)/rc0.d/K99nic

