#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/intftools.d/add.sh	1.5.6.2"
#ident  "$Header: add.sh 1.2 91/06/21 $"
# Program invokes bkreg with -a option to add a line to a backup register.

TABLE="$1"
TAG="$2"
PRI="$3"
ONAME="$4"
ODEV="$5"
OMNAME="$6"
WEEKS="$7"
DAYS="$8"
METHOD="$9"

shift 9
MOPTS="$1"
DEPEND="$2"
DGROUP="$3"
DDEV="$4"
DCHARS="$5"
DMNAMES="$6"

SELECT=
OPTS=

if [ "$WEEKS" = "all" ]
then
	PD=`getrpd $TABLE`
	WEEKS="1-$PD"
fi

if [ "$DAYS" = "all" ]
then
	DAYS="0-6"
fi

if [ "$WEEKS" = "demand" ]
then
	SELECT="$WEEKS"
else
	SELECT="$WEEKS:$DAYS"
fi

DEST="$DGROUP:$DDEV:$DCHARS:$DMNAMES"

ORIG="$ONAME:$ODEV:$OMNAME"

if [ -n "$MOPTS" ]
then
	OPTS="-b \"$MOPTS\""
fi

if [ -n "$DEPEND" ]
then
	OPTS="$OPTS -D \"$DEPEND\""
fi

eval bkreg -a "$TAG" -t "$TABLE" -c \"$SELECT\" -m "$METHOD" -d \"$DEST\" -o \"$ORIG\" -P "$PRI" "$OPTS" 2>&1
RC=$?

if [ $RC -eq 0 ]
then
	echo Entry successfully added to backup register.
else
	exit $RC
fi
