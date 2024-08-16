#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/rcmds.d/add_cron.sh	1.3"
#ident	"$Header: $"

task=$1
month=$2
date=$3
day=$4
hour=$5
minute=$6
tmp=/tmp/addcron.$VPID

if [ "$month" = "all" ]
then	month='*'
fi

if [ "$date" = "all" ]
then	date='*'
fi

if [ "$day" = "all" ]
then	day='*'
fi

if [ "$hour" = "all" ]
then	hour='*'
fi

if [ "$minute" = "all" ]
then 	minute='*'
fi

# Backup will be on the tape device ctape1 defined in /etc/device.tab.
if [ "$task" = "System Backup" ]
then task="echo '\\\n' | /usr/bin/backup -c $device"
elif [ "$task" = "Incremental System Backup" ]
then task="echo '\\\n' | /usr/bin/backup -p $device"
else task=`cat $task`
fi

crontab -l > $tmp
echo "$minute $hour $date $month $day $task" >> $tmp
crontab $tmp 
rm $tmp
