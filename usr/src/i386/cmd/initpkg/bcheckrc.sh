#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)initpkg:i386/cmd/initpkg/bcheckrc.sh	1.5.33.19"

if [ -z "$LC_ALL" -a -z "$LC_MESSAGES" ]
then
	if [ -z "$LANG" ]
	then
		LNG=`defadm locale LANG 2>/dev/null`
		if [ "$?" != 0 ]
		then LANG=C
		else eval $LNG
		fi
	fi
	LC_MESSAGES=/etc/inst/locale/$LANG
	export LANG LC_MESSAGES
fi
LABEL="UX:$0"

CAT=uxrc; export CAT

# This file has those commands necessary to check the file
# system, date, and anything else that should be done before mounting
# the file systems.

rootfs=/dev/root
rrootfs=/dev/rroot
vfstab=/etc/vfstab

test -x /sbin/dumpcheck && /sbin/dumpcheck

# Try to get the file system type for root.  If this fails,
# it will be set to null.  In that case, we hope that vfstab
# is around so that fsck will use it for the type.

# Set the console state to public.
if [ -f /sbin/consalloc ]
then
	/sbin/consalloc 2
fi

# put root into mount table
/sbin/setmnt "${rootfs} /" </dev/null&

# Initialize node name and system name from names stored
# in /etc/nodename and /etc/systemid
[ -s /etc/nodename ] && read node </etc/nodename && [ -n "$node" ] &&
      NODE="-n $node"
[ -s /etc/systemid ] && read sysid </etc/systemid && [ -n "$sysid" ] &&
      SYSID="-s $sysid"
if [ -n "$NODE" -o -n "$SYSID" ]
then
	/sbin/setuname $NODE $SYSID
fi
if [ -n "$node" ]
then
	pfmt -s nostd -g $CAT:47 "Node: %s\n" $node
else
	pfmt -s nostd -g $CAT:47 "Node: %s\n" "`/sbin/uname -n`"
fi

exec >/dev/null 2>&1

wait

/sbin/mount /proc&
/sbin/mount /dev/fd&

while read bdevice rdevice mntp fstype fsckpass automnt mntopts
do
	# check for comments
	case ${bdevice} in
	'#'*)	continue
	esac

	# see if this is /stand or /var - check and mount if it is
	if [ "${mntp}" = "/stand" -o "${mntp}" = "/var" ]
	then
		/sbin/mount ${mntp}
		if [ $? -ne 0 ]
		then
			/sbin/fsck -F ${fstype} -m  ${rdevice}
			if [ $? -ne 0 ]
			then
				pfmt -l $LABEL -s info -g $CAT:48 "The %s file system (%s) is being checked.\n" $mntp ${rdevice} 2>/dev/console
				/sbin/fsck -F ${fstype} -y  ${rdevice}
			fi
			/sbin/mount ${mntp}
		fi
	fi
done < $vfstab
wait
