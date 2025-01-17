#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:i386/cmd/oamintf/files/bin/getchoimod.sh	1.1.2.1"
#ident	"$Header: $"

> /tmp/choices.$$
for des in `/usr/sadm/sysadm/bin/getdefdev | grep -v ALL | sort`;
do
	grep "$des" /tmp/choices.$$ > /dev/null 2>&1
	if [ $? != 0 ]
	then
		nam=`echo "$des" | sed 's/^.*dsk\///'`
		chr=`echo "$nam" | sed 's/^\(.\).*/\1/'`
		if [ "$chr" = "f" ]
		then
			devalias=`devattr $des alias`
			echo "$des\072$devalias"
 		elif [ "$chr" = "c" ]
		then
			slice=`echo "$nam" | sed 's/^.*s//'`
			device=`echo "$nam" | sed 's/s.*/s0/'`
			desc="`devattr /dev/rdsk/"$device" alias` slice $slice"
			echo "$des\072$desc"
		else
			echo "$des\072$des"
		fi
		echo "$des" >> /tmp/choices.$$
	fi
done
rm -f /tmp/choices.$$
