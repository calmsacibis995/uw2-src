#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-nm:common/cmd/cmd-nm/usr.sbin/snmp/snmp/snmp.mk	1.3"
#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nm/usr.sbin/snmp/snmp/snmp.mk,v 1.3 1994/05/24 17:47:21 cyang Exp $"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	STREAMware TCP
#	Copyright 1987, 1993 Lachman Technology, Inc.
#	All Rights Reserved.
#

#
# Copyrighted as an unpublished work.
# (c) Copyright 1989 INTERACTIVE Systems Corporation
# All rights reserved.
#

#      @(#)snmp.mk	1.1 INTERACTIVE SNMP  source

include ${CMDRULES}

RC0=67
RC1=67
RC2=73

RC0D = ${ETC}/rc0.d
RC1D = ${ETC}/rc1.d
RC2D = ${ETC}/rc2.d
INITD = ${ETC}/init.d
OWN= bin
GRP= bin


all: snmp

install: all
	[ -d $(INITD) ] || mkdir -p $(INITD)
	${INS} -f ${INITD} -m 0555 -u ${OWN} -g ${GRP} snmp
	rm -f ${RC0D}/K${RC0}snmp
	rm -f ${RC1D}/K${RC1}snmp
	rm -f ${RC2D}/S${RC2}snmp
	[ -d $(RC0D) ] || mkdir -p $(RC0D)
	ln ${INITD}/snmp ${RC0D}/K${RC0}snmp
	[ -d $(RC1D) ] || mkdir -p $(RC1D)
	ln ${INITD}/snmp ${RC1D}/K${RC1}snmp
	[ -d $(RC2D) ] || mkdir -p $(RC2D)
	ln ${INITD}/snmp ${RC2D}/S${RC2}snmp

clean:

clobber: clean
