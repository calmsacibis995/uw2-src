#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cpq.cmds:cpqsmu/cpqsmu.mk	1.3"

include $(CMDRULES)

MAKEFILE  = cpqsmu.mk
CPQ_INS	  = $(USRBIN)/compaq/diags/cpqscsi
LDLIBS	  = -lcurses -lc -lw

# CPQ_DEBUG - Enables the debugging output. (OFF)
# CPQ_PHASE4 - Enables the code which will be released for phase 4 of
#	the agreement with Compaq. (OFF)
# CPQ_SCSI - Enables the SCSI specific code. (ON)
# CPQ_IDA - Enables the IDA specific code. (OFF)
# UNIXWARE - It enables the Unixware changes in the
#	Compaq code. (ON)

LOCALDEF  = -DCPQ_SCSI -DUNIXWARE
LOCALINC  = -I.

HFILES    = cled.h cpqsmu.h \
	    globals.h helptxt.h nvm.h scsicmd.h \
	    scsidef.h scsi.h scsiinq.h scsisup.h \
	    sysdefs.h unixware.h win.h
CFILES	  = acti.c basewin.c dela.c diag.c errors.c \
	    help.c helpscr.c init.c inte.c main.c \
	    para.c parse.c scroll.c stat.c syntax.c \
	    unixware.c workwin.c
OFILES	  = acti.o basewin.o dela.o diag.o errors.o \
	    help.o helpscr.o init.o inte.o main.o \
	    para.o parse.o scroll.o stat.o syntax.o \
	    unixware.o workwin.o
TARGET	  = cpqsmu
PROBEFILE = main.c

all:
	@if [ -f $(PROBEFILE) ]; then \
		find $(TARGET) \( ! -type f -o -links +1 \) \
			-exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
		$(MAKE) -f $(MAKEFILE) $(TARGET) $(MAKEARGS) ;\
	else \
		if [ ! -r $(TARGET) ]; then \
			echo "ERROR: $(TARGET) is missing" 1>&2 ;\
			false ;\
			break ;\
		fi \
	fi

clean:
	$(RM) -f $(OFILES)

clobber:
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(OFILES)" ;\
		rm -f $(OFILES) ;\
	fi

install: all
	[ -d $(CPQ_INS) ] || mkdir -p $(CPQ_INS)
	$(INS) -f $(CPQ_INS) -m 0750 -u root -g bin cpqsmu

$(TARGET): $(OFILES)
	$(CC) -o $@ $(OFILES) $(LDFLAGS) $(LDLIBS) $(LOCALINC)

include cpqsmu.mk.deps
