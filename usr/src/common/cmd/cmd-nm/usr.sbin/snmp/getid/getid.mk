#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-nm:common/cmd/cmd-nm/usr.sbin/snmp/getid/getid.mk	1.5"
#ident "$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nm/usr.sbin/snmp/getid/getid.mk,v 1.5 1994/06/24 16:11:06 rbell Exp $"
# Copyrighted as an unpublished work.
# (c) Copyright 1989, 1990 INTERACTIVE Systems Corporation
# All rights reserved.
#

#      @(#)getid.mk	1.5 INTERACTIVE SNMP  source

# Copyright (c) 1987, 1988 Kenneth W. Key and Jeffrey D. Case


include ${CMDRULES}

LOCALINC=-I$(INC)/netmgt
LOCALDEF=-DSVR4
LDLIBS = -lsocket -lnsl -lsnmp -lsnmpio

INCLUDES = $(INC)/netmgt/snmp.h $(INC)/netmgt/snmpuser.h

INSDIR = ${USRSBIN}
OWN = bin
GRP = bin

all: getid

getid: getid.o 
	${CC} -o getid ${LDFLAGS} getid.o ${LIBS} ${LDLIBS} ${SHLIBS}

getid.o: getid.c ${INCLUDES}

install: all
	${INS} -f ${INSDIR} -m 0555 -u ${OWN} -g ${GRP} getid
	[ -d ${USRLIB}/locale/C/MSGFILES ] || \
		mkdir -p ${USRLIB}/locale/C/MSGFILES
	${INS} -f ${USRLIB}/locale/C/MSGFILES -m 0666 -u ${OWN} -g ${GRP} nmgetid.str
clean:
	rm -f getid.o

clobber: clean
	rm -f getid
