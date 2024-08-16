#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cpq.cmds:cpqscsimon/cpqscsimon.mk	1.5"

include $(CMDRULES)

MAKEFILE  = cpqscsimon.mk
SNMP	  = ../snmp
CPQ_INS	  = $(USRBIN)/compaq/diags/cpqscsi
STDEQ	  = $(SNMP)/stdeq
DEFINE	  = $(SNMP)/define
LIB	  = $(SNMP)/lib
SMUX	  = $(SNMP)/smux
CIPC	  = $(SNMP)/cipc/src
LDLIBS	  = $(LIB)/libmon.a -lelf
SCSI	  = ../cpqsmu

# CPQ_DEBUG - Enables the debugging output. (OFF)
# CPQ_PHASE4 - Enables the code which will be released for phase 4 of
#	the agreement with Compaq. (OFF)
# CPQ_SCSI - Enables the SCSI specific code. (ON)
# CPQ_IDA - Enables the IDA specific code. (OFF)
# UNIXWARE - It enables the Unixware changes in the
#	Compaq code. (ON)

LOCALDEF  = -DGENERIC -Dfprintf=agentlog_fprintf -DCOMPAQ -DCPQ_SCSI -DUNIXWARE
LOCALINC  = -I. -I$(SCSI) -I$(LIB) -I$(DEFINE) -I$(SMUX) -I$(CIPC) -I$(STDEQ)

HFILES	  = cpqscsi.h cpqstsys.h logicproto.h
CFILES	  = agent.c logic.c unixware.c
OFILES	  = agent.o logic.o unixware.o
TARGET	  = cpqscsimon
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
	$(INS) -f $(CPQ_INS) -m 0750 -u root -g other cpqscsimon
	$(INS) -f $(ETC)/rc2.d -m 0750 -u root -g bin S99cpqscsimon
	-$(RM) -f $(ETC)/cpqscsimon
	-$(RM) -f $(ETC)/rc0.d/K01cpqscsimon
	-ln $(ETC)/rc2.d/S99cpqscsimon $(ETC)/cpqscsimon
	-ln $(ETC)/rc2.d/S99cpqscsimon $(ETC)/rc0.d/K01cpqscsimon

# Copy common source file from another directory
unixware.c: $(SCSI)/unixware.c
	rm -f unixware.c
	cp $(SCSI)/unixware.c .

$(TARGET): $(OFILES)
	$(CC) -o $@ $(LIB)/main.o $(OFILES) $(STDEQ)/ev.o $(LDFLAGS) $(LDLIBS) $(LOCALINC)

agent.o:$(DEFINE)/define.h
agent.o:$(LIB)/cipc.h
agent.o:$(SMUX)/router.h
agent.o:$(LIB)/queue.h
agent.o:$(LIB)/log.h
agent.o:$(LIB)/obj.h
logic.o:$(SCSI)/scsi.h
logic.o:$(SCSI)/scsicmd.h
logic.o:$(SCSI)/cled.h
logic.o:$(LIB)/obj.h
logic.o:cpqscsi.h
logic.o:cpqstsys.h
logic.o:$(SCSI)/scsiinq.h
logic.o:$(SCSI)/scsidef.h
logic.o:$(SCSI)/unixware.h
