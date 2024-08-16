#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-nm:common/cmd/cmd-nm/usr.sbin/snmp/trap/trap.mk	1.4"
# Copyrighted as an unpublished work.
# (c) Copyright 1989 INTERACTIVE Systems Corporation
# All rights reserved.
#

#      @(#)trap.mk	1.4 INTERACTIVE SNMP  source

# Copyright (c) 1987, 1988 Kenneth W. Key and Jeffrey D. Case

include ${CMDRULES}

LOCALINC=-I$(INC)/netmgt
LOCALDEF=-DSVR4
LDLIBS = -lsocket -lnsl -lsnmp 

INCLUDES = $(INC)/netmgt/snmp.h $(INC)/netmgt/snmpuser.h

INSDIR = ${USRSBIN}
OWN= bin
GRP= bin

all: trap_rece trap_send

trap_rece: trap_rece.o ${LIBS}
	${CC} -o trap_rece trap_rece.o ${LDFLAGS} ${LIBS} ${LDLIBS} ${SHLIBS}

trap_send: trap_send.o ${LIBS}
	${CC} -o trap_send trap_send.o ${LDFLAGS} ${LIBS} ${LDLIBS} ${SHLIBS}

trap_rece.o: trap_rece.c ${INCLUDES}

trap_send.o: trap_send.c ${INCLUDES}

install: all
	${INS} -f ${INSDIR} -m 0555 -u ${OWN} -g ${GRP} trap_rece
	${INS} -f ${INSDIR} -m 0555 -u ${OWN} -g ${GRP} trap_send
	[ -d ${USRLIB}/locale/C/MSGFILES ] || \
		mkdir -p ${USRLIB}/locale/C/MSGFILES
	${INS} -f ${USRLIB}/locale/C/MSGFILES -m 0666 -u ${OWN} -g ${GRP} nmtrap.str

clean:
	rm -f trap_rece.o trap_send.o *~

clobber: clean
	rm -f trap_rece trap_send
