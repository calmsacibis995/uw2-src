#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-nm:common/cmd/cmd-nm/usr.sbin/snmp/snmpstat/snmpstat.mk	1.5"
#ident "$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nm/usr.sbin/snmp/snmpstat/snmpstat.mk,v 1.5 1994/06/24 16:11:34 rbell Exp $"
# Copyrighted as an unpublished work.
# (c) Copyright 1989, 1990 INTERACTIVE Systems Corporation
# All rights reserved.
#

#      @(#)snmpstat.mk	2.1 INTERACTIVE SNMP  source

# Copyright (c) 1987, 1988 Kenneth W. Key and Jeffrey D. Case

include ${CMDRULES}

LOCALINC=-I$(INC)/netmgt
LOCALDEF=-DSVR4

OBJS = main.o util.o tcp.o route.o at.o system.o if.o udp.o snmp.o

LDLIBS = -lsocket -lnsl -lsnmp -lsnmpio 

INCLUDES = $(INC)/netmgt/snmpio.h

INSDIR = ${USRSBIN}
OWN= bin
GRP= bin

all: snmpstat

snmpstat: ${OBJS} ${LIBS}
	${CC} -o snmpstat ${LDFLAGS} ${OBJS} ${LIBS} ${LDLIBS} ${SHLIBS} 

main.o: main.c
util.o: util.c ${INCLUDES}
route.o: route.c ${INCLUDES}
tcp.o: tcp.c ${INCLUDES}
at.o: at.c ${INCLUDES}
system.o: system.c ${INCLUDES}
if.o: if.c ${INCLUDES}
udp.o: udp.c ${INCLUDES}
snmp.o: snmp.c ${INCLUDES}

install: all
	${INS} -f ${INSDIR} -m 0555 -u ${OWN} -g ${GRP} snmpstat
	[ -d ${USRLIB}/locale/C/MSGFILES ] || \
		mkdir -p ${USRLIB}/locale/C/MSGFILES
	${INS} -f ${USRLIB}/locale/C/MSGFILES -m 0666 -u ${OWN} -g ${GRP} nmsnmpstat.str

clean:
	rm -f ${OBJS} *~

clobber: clean
	rm -f snmpstat
