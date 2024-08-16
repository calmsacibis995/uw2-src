#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/interface/rmstldvrs.sh	1.2"
#ident  "$Header: $"

# Remove stale device reservations from the devreserv(1) reservations file.

# Stale device reservations are reservations made by now-defunct sysadm 
# processes.  Reservations made by sysadm processes for storage device 
# operations such as copy, erase, format, and remove are identified by 
# devreserv keys of the form 21474ppppp, formed by prefixing a special tag 
# of 21474 to the process id ppppp, right adjusted and padded with 0's.  
# E.g. if the pid is 7 the key is 2147400007.

specialkeylen=10
taglen=5
begpid=6
tag=21474

devreserv >/tmp/oaminit$$ 2>/dev/null

while read device key
do
	keylen=`expr length $key`
	if [ $keylen -eq $specialkeylen ]
	then
		prefix=`expr substr $key 1 $taglen`
		if [ $prefix -eq $tag ]
		then
			pid=`expr substr $key $begpid $keylen`
			pstate=`ps -l -p $pid | grep : | cut -c4`
			if [ "$pstate" = "" -o "$pstate" = "Z" ]
			then
				devfree $key $device >/dev/null
			fi
		fi
	fi
done </tmp/oaminit$$

rm -f /tmp/oaminit$$
