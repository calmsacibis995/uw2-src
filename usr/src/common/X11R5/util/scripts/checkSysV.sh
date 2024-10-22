#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)r5util:scripts/checkSysV.sh	1.1"
#!/bin/sh

case "$1" in
"")	echo "Usage: $0 directory"; exit 1;;
esac

echo "Analyzing $1 for Incompatabilities with System V"

echo 'File names longer than 12 characters (excluding the doc directory):'
cd $1

dirlist=
for dir in `echo *`
do
	case "$dir" in
	doc)	;;
	*)	dirlist="$dirlist $dir";;
	esac
done

(
	find doc      -name '???????????????*' -print
	find $dirlist -name '?????????????*' -print
) | sort \
  | sed -e '/,v/d' \
		-e 's/^/	/'

echo 'Symbolic links:'
find . -type l -print | sed -e 's/^/	/'

