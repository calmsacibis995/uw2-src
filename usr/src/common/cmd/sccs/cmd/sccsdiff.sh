#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)sccs:cmd/sccsdiff.sh	6.5.1.3"
#	DESCRIPTION:
#		Execute bdiff(1) on two versions of a set of
#		SCCS files and optionally pipe through pr(1).
#		Optionally specify bdiff segmentation size.

trap "rm -f /tmp/get[abc]$$;exit 1" 0 1 2 3 15

if [ $# -lt 3 ]
then
	pfmt -l "UX:sccsdiff" -s action -g uxepu:147 "Usage: sccsdiff -r<sid1> -r<sid2> [-p] [-s<num-arg>] sccsfile ...\n"
	exit 1
fi

for i in $@
do
	case $i in

	-*)
		case $i in

		-r*)
			if [ ! "$sid1" ]
			then
				sid1=`echo $i | sed -e 's/^-r//'`
			elif [ ! "$sid2" ]
			then
				sid2=`echo $i | sed -e 's/^-r//'`
			fi
			;;
		-s*)
			num=`echo $i | sed -e 's/^-s//'`
			;;
		-p*)
			pipe=yes
			;;
		*)
			pfmt -l "UX:sccsdiff" -s error -g uxepu:148 "unknown argument: %s\n" $i
			exit 1
			;;
		esac
		;;
	*s.*)
		files="$files $i"
		;;
	*)
		pfmt -l "UX:sccsdiff" -s error -g uxepu:149 "%s not an SCCS file\n" $i
		;;
	esac
done

for i in $files
do
	if get -s -p -k -r$sid1 $i > /tmp/geta$$
	then
		if get -s -p -k -r$sid2 $i > /tmp/getb$$
		then
			bdiff /tmp/geta$$ /tmp/getb$$ $num > /tmp/getc$$
		fi
	fi
	if [ ! -s /tmp/getc$$ ]
	then
		if [ -f /tmp/getc$$ ]
		then
			pfmt -l "UX:sccsdiff" -s error -g uxepu:150 "%s: No differences\n" $i 2> /tmp/getc$$
		else
			exit 1
		fi
	fi
	if [ "$pipe" ]
	then
		pr -h "$i: $sid1 vs. $sid2" /tmp/getc$$
	else
		cat /tmp/getc$$
	fi
done

trap 0
rm -f /tmp/get[abc]$$
