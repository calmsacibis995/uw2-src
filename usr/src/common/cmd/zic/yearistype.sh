#! /bin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#       Copyright (c) 1993
#         All Rights Reserved

#       THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Univel
#       The copyright notice above does not evidence any
#       actual or intended publication of such source code.
 
#ident	"@(#)zic:yearistype.sh	1.1"

case $#-$2 in
	2-odd)	case $1 in
			*[13579])	exit 0 ;;
			*)		exit 1 ;;
		esac ;;
	2-even)	case $1 in
			*[24680])	exit 0 ;;
			*)		exit 1 ;;
		esac ;;
	2-*)	echo "$0: wild type - $2" >&2
		exit 1 ;;
	*)	echo "$0: usage is $0 year type" >&2
		exit 1 ;;
esac
