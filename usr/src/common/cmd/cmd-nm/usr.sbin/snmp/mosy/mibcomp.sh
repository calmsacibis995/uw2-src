#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-nm:common/cmd/cmd-nm/usr.sbin/snmp/mosy/mibcomp.sh	1.2"
#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nm/usr.sbin/snmp/mosy/mibcomp.sh,v 1.2 1994/05/24 19:42:26 cyang Exp $"
:
#      @(#)mibcomp.sh	1.1 STREAMWare TCP/IP SVR4.2  source
:
#      @(#)mibcomp.sh	6.1 Lachman System V STREAMS TCP  source
#      SCCS IDENTIFICATION
#
# Copyrighted as an unpublished work.
# (c) Copyright 1992 INTERACTIVE Systems Corporation
# All rights reserved.
#
: run this script through /bin/sh
BIN="/usr/sbin"
sflag=
OUTPUT=
while getopts so: C; do
	case $C in
	s)
		sflag=-s
		;;
	o)
		OUTPUT=$OPTARG
		;;
	\?)
		echo "usage: $0: [-s] -o output-file mib-file..."
		exit 1
		;;
	esac
done
shift `expr $OPTIND - 1`
if [ $# = 0 -o X$OUTPUT = X ]; then
	echo "usage: $0: [-s] -o output-file mib-file..."
	exit 1
fi
FILES=$*
OFILES=
for i in $FILES; do
	curfile=`basename $i \.my`.defs
	OFILES=${OFILES}" "$curfile
	${BIN}/mosy $sflag -o $curfile $i
done
cat $OFILES > /tmp/mc$$
${BIN}/post_mosy -i /tmp/mc$$ -o $OUTPUT
rm -f /tmp/mc$$
