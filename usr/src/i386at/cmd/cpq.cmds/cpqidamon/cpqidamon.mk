#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cpq.cmds:cpqidamon/cpqidamon.mk	1.4"

include $(CMDRULES)

MAKEFILE  = cpqidamon.mk
SNMP	  = ../snmp
CPQ_INS	  = $(USRBIN)/compaq/diags/ida
LIB	  = $(SNMP)/lib
DEFINE	  = $(SNMP)/define
SMUX	  = $(SNMP)/smux
CIPC	  = $(SNMP)/cipc/src
LDLIBS	  = $(LIB)/libmon.a -lelf
SCSI	  = ../cpqsmu

# CPQ_DEBUG - Enables the debugging output. (OFF)
# CPQ_PHASE4 - Enables the code which will be released for phase 4 of
#	the agreement with Compaq. (OFF)
# CPQ_SCSI - Enables the SCSI specific code. (OFF)
# CPQ_IDA - Enables the IDA specific code. (ON)
# UNIXWARE - It enables the Unixware changes in the
#	Compaq code. (ON)

LOCALDEF  = -Dfprintf=agentlog_fprintf -DCOMPAQ -DGENERIC -DCPQ_IDA -DUNIXWARE
LOCALINC  = -I. -I$(SCSI) -I$(LIB) -I$(DEFINE) -I$(SMUX) -I$(CIPC)

HFILES	  = idadef.h misc.h
CFILES	  = agent.c common.c ida.c misc.c unixware.c
OFILES	  = agent.o common.o ida.o misc.o unixware.o
TARGET	  = cpqidamon
PROBEFILE = agent.c

all:
	@if [ -f $(PROBEFILE) ]; then \
		find $(TARGET) \( ! -type f -o -links +1 \) \
			-exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
		$(MAKE) -f $(MAKEFILE) unixware.c ;\
		$(MAKE) -f $(MAKEFILE) $(TARGET) $(MAKEARGS) ;\
	else \
		if [ ! -r $(TARGET) ]; then \
			echo "ERROR: $(TARGET) is missing" 1>&2 ;\
			false ;\
			break ;\
		fi \
	fi

clean:
	rm -f $(OFILES)

clobber:
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(OFILES)" ;\
		rm -f $(OFILES) ;\
		rm -f unixware.c ;\
	fi

install: all
	[ -d $(CPQ_INS) ] || mkdir -p $(CPQ_INS)
	[ -d $(ETC)/rc2.d ] || mkdir -p $(ETC)/rc2.d
	[ -d $(ETC)/rc0.d ] || mkdir -p $(ETC)/rc0.d
	$(INS) -f $(CPQ_INS) -m 0750 -u root -g other cpqidamon
	$(INS) -f $(ETC)/rc2.d -m 0750 -u root -g bin S99cpqidamon
	-$(RM) -f $(ETC)/cpqidamon
	-$(RM) -f $(ETC)/rc0.d/K01cpqidamon
	-ln $(ETC)/rc2.d/S99cpqidamon $(ETC)/cpqidamon
	-ln $(ETC)/rc2.d/S99cpqidamon $(ETC)/rc0.d/K01cpqidamon

# Copy common source file from another directory
unixware.c: $(SCSI)/unixware.c
	rm -f unixware.c
	cp $(SCSI)/unixware.c .

$(TARGET): $(OFILES)
	$(CC) -o $@ $(LIB)/main.o $(OFILES) $(LDFLAGS) $(LDLIBS) $(LOCALINC)

agent.o:$(DEFINE)/define.h
agent.o:$(LIB)/cipc.h
agent.o:$(SMUX)/router.h
agent.o:$(LIB)/queue.h
agent.o:$(LIB)/log.h
agent.o:$(LIB)/obj.h
agent.o:$(LIB)/lib.h
agent.o:idadef.h
common.o:$(DEFINE)/define.h
common.o:$(LIB)/obj.h
ida.o:$(DEFINE)/define.h
ida.o:$(LIB)/obj.h
ida.o:idadef.h
ida.o:$(SCSI)/unixware.h
misc.o:$(DEFINE)/define.h
misc.o:$(LIB)/obj.h
misc.o:idadef.h
misc.o:$(DEFINE)/agentmsg.h
misc.o:misc.h
misc.o:$(SCSI)/unixware.h
