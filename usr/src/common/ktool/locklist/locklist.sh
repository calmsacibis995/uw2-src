#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ktool:common/ktool/locklist/locklist.sh	1.3"

if [ ! -f unix.mk ]
then
	echo "locklist must be run from the directory containing unix.mk" >&2
	exit 1
fi

TMPINC=/tmp/inc$$
trap 'rm -rf $TMPINC;trap 0;exit' 0 1 2 3 12

mkdir $TMPINC $TMPINC/util

cat >$TMPINC/util/ksynch.h <<%EOF%
/*
 * Special version of util/ksynch.h to generate info from LKINFO_DECL()
 * and *_INIT() macros.
 */
#ifndef _UTIL_KSYNCH_H
#define _UTIL_KSYNCH_H

#ifndef _UTIL_TYPES_H
#include <util/types.h>
#endif
#ifndef _UTIL_DL_H
#include <util/dl.h>
#endif
#ifndef _UTIL_IPL_H
#include <util/ipl.h>
#endif
#ifndef _UTIL_LIST_H
#include <util/list.h>
#endif

#define pl0	PL0
#define pl1	PL1
#define pl2	PL2
#define pl3	PL3
#define pl4	PL4
#define pl5	PL5
#define pl6	PL6
#define pl7	PL7
#define plbase	PLBASE
#define pltimeout PLTIMEOUT
#define pldisk	PLDISK
#define plstr	PLSTR
#define plhi	PLHI

#define LKINFO_DECL(var, name, flags)	({"DECL":var,name,})

#define FSPIN_INIT(lp)          	({"FSPIN":lp})
#define LOCK_INIT(lp, h, m, p, s)	({"SPIN":lp,h,m,p})
#define RW_INIT(lp, h, m, p, s)		({"RWSPIN":lp,h,m,p})

#define SLEEP_INIT(l, h, p, s)		({"SLEEP":l,h,p})
#define RWSLEEP_INIT(l, h, p, s)	({"RWSLEEP":l,h,p})

#endif /* _UTIL_KSYNCH_H */
%EOF%

echo "working...\c" >&2

P_LOCKLIST=/usr/lib/${PFX}_locklist
LOCKLIST=$ROOT/$MACH/$P_LOCKLIST
[ -x $LOCKLIST -o ! -x $TOOLS/$P_LOCKLIST ] || LOCKLIST=$TOOLS/$P_LOCKLIST

for f in `${PFX}make -P -f unix.mk fnames | grep '\.c$'`
do
	echo ".\c" >&2
	${PFX}cc -E -D_KERNEL -I$TMPINC -I. $f |
		sed -n 's/&//g;/( *{/s/.*( *{ *"\([^"]*\)"\(.*\) *} *).*/\1\2/p'
done | $LOCKLIST
