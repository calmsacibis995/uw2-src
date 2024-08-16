#!/bin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/refer/roffbib.sh	1.2"
#ident	"$Header: $"

#       Copyright(c) 1988, Sun Microsystems, Inc.
#       All Rights Reserved


#
#	roffbib sh script
#
flags=
abstr=
headr=BIBLIOGRAPHY
xroff=nroff
macro=-mbib

for i
do case $1 in
	-[onsrT]*|-[qeh])
		flags="$flags $1"
		shift ;;
	-x)
		abstr=-x
		shift ;;
	-m)
		shift
		macro="-m$1"
		shift ;;
	-Q)
		xroff="troff"
		shift ;;
	-H)
		shift
		headr="$1"
		shift ;;
	-*)
		echo "roffbib: unknown flag: $1"
		shift
	esac
done
if test $1
then
	(echo .ds TL $headr; refer -a1 -B$abstr $*) | $xroff $flags $macro
else
	(echo .ds TL $headr; refer -a1 -B$abstr) | $xroff $flags $macro
fi
