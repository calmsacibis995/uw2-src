#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)dfs.cmds:shareall/shareall.sh	1.5.7.4"
#ident  "$Header: shareall.sh 1.2 91/06/26 $"
# shareall  -- share resources

LABEL="UX:shareall"
CATALOG="uxdfscmds"

USAGE="Usage: shareall [-F fsys[,fsys...]] [- | file]\n"
fsys=

while getopts F: i
do
	case $i in
	F)  fsys=$OPTARG;;
	\?) pfmt -l $LABEL -s action -g $CATALOG:11 "$USAGE" >&2; exit 1;;
	esac
done
shift `expr $OPTIND - 1`

if [ $# -gt 1 ]		# accept only one argument
then
	pfmt -l $LABEL -s action -g $CATALOG:11 "$USAGE" >&2
	exit 1
elif [ $# = 1 ]
then
	case $1 in
	-)	infile=;;	# use stdin
	*)	infile=$1;;	# use a given source file
	esac
else
	infile=/etc/dfs/dfstab	# default
fi


if [ "$fsys" ]		# for each file system ...
then
	`egrep "^[^#]*[ 	][ 	]*-F[ 	]*(\`echo $fsys|tr ',' '|'\`)" $infile|/sbin/sh`
else			# for every file system ...
	cat $infile|/sbin/sh
fi
