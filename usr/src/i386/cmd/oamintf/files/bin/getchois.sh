#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:i386/cmd/oamintf/files/bin/getchois.sh	1.2.2.1"
#ident	"$Header: $"

if [ $# = 1 ]
then
	dev=$1
else
	dev=cdevice
fi

for des in `/usr/sadm/sysadm/bin/dev $dev`;
do
	nam=`echo "$des" | sed 's/^.*dsk\///'`
	chr=`echo "$nam" | sed 's/^\(.\).*/\1/'`
	if [ "$chr" = "f" ]
	then
		echo "$des\072\c"; devattr $des alias
	else
	slice=`echo "$nam" | sed 's/^.*s//'`
	device=`echo "$nam" | sed 's/s.*/s0/'`
	desc="`devattr /dev/rdsk/"$device" alias` slice $slice"
	echo "$des\072$desc"
	fi
done
