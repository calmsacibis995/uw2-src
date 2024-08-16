#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-nm:common/cmd/cmd-nm/usr.sbin/snmp/getnext/getnext.mk	1.5"
#ident "$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nm/usr.sbin/snmp/getnext/getnext.mk,v 1.5 1994/06/24 16:11:13 rbell Exp $"
# Copyrighted as an unpublished work.
# (c) Copyright 1989, 1990 INTERACTIVE Systems Corporation
# All rights reserved.
#

#      @(#)getnext.mk	1.5 INTERACTIVE SNMP  source

# Copyright (c) 1987, 1988 Kenneth W. Key and Jeffrey D. Case


include ${CMDRULES}

LOCALDEF=-DSVR4
LOCALINC=-I$(INC)/netmgt

LDLIBS = -lsocket -lnsl -lsnmp -lsnmpio

INCLUDES = $(INC)/netmgt/snmp.h $(INC)/netmgt/snmpuser.h

INSDIR = ${USRSBIN}
OWN = bin
GRP = bin

all: getnext

getnext: getnext.o ${LIBS}
	${CC} -o getnext ${LDFLAGS} getnext.o ${LIBS} ${LDLIBS} ${SHLIBS}

getnext.o: getnext.c ${INCLUDES}

install: all
	${INS} -f ${INSDIR} -m 0555 -u ${OWN} -g ${GRP} getnext
	[ -d ${USRLIB}/locale/C/MSGFILES ] || \
		mkdir -p ${USRLIB}/locale/C/MSGFILES
	${INS} -f ${USRLIB}/locale/C/MSGFILES -m 0666 -u ${OWN} -g ${GRP} nmgetnext.str

clean:
	rm -f getnext.o

clobber: clean
	rm -f getnext
