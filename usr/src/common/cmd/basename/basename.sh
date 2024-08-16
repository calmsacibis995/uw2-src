#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)basename:basename.sh	1.8.1.5"
#ident  "$Header: basename.sh 1.2 91/06/26 $"
if [ $# -gt 2 ]
then
	catalog=uxcore
	label=UX:basename
	pfmt -l $label -g $catalog:1 "Incorrect usage\\n"
	pfmt -l $label -g $catalog:2 -s action "Usage: basename [ path [ suffix-pattern ] ]\\n"
	exit 1
fi
#	If no first argument or first argument is null, make first argument
#	"."  Add beginning slash, then remove trailing slashes, then remove 
#	everything up through last slash, then remove suffix pattern if 
#	second argument is present.
#	If nothing is left, first argument must be of form //*, in which
# 	case the basename is /.
exec /usr/bin/sed \
	-e 's!/*$!!' \
	-e 's!.*/!!' \
	-e "s!\\(.\\)$2\$!\\1!" \
	-e 's!^$!/!' <<!
/${1:-.}
!
