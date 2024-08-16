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

#ident	"@(#)bkrs:common/cmd/bkrs/rcmds.d/filemsg.sh	1.2"
#ident	"$Header: $"
if [ "$2" = "" ]
then
	echo You must provide a valid file or directory name or strike CANCEL.
	exit
fi 

f=""

for i in $2
do

	if [ "$1" = "System" ]
	then
		if test -f $i -o -d $i
		then
			:
		else 
			f="${f},${i}"
		fi
	elif [ "$1" = "Personal" ]
	then
		if [ "$i" = "/" -a "$HOME" != "/" ]
		then
			echo "You may only backup files or directories in $HOME."
			exit
		fi

		if test -f $HOME/$i -o -d $HOME/$i
		then
			:
		else
			if echo "$i" | grep "$HOME" > /dev/null
			then
				:
			else
				f="${f},${i}"
				exit
			fi
		fi
	fi
done
if [ -n "$f" ]
then
        f=`echo "$f" | sed "s/^,//"`
        if [ "$1" = "System" ]
        then echo "$f cannot be found."
        else echo "$f cannot be found in home directory."
        fi
fi
