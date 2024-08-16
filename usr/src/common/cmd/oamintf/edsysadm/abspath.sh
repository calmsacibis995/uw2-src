#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)oamintf:common/cmd/oamintf/edsysadm/abspath.sh	1.1"
#ident  "$Header: abspath.sh 2.0 91/07/12 $"

LOC=$1
NAME=$2
FILE=$3
PARENTMENU=`$OAMBASE/edbin/action $LOC`
PARENTLOC=`dirname $PARENTMENU | sed "s;$INTFBASE;;"`
PKGINST=`grep "^$NAME\^" $PARENTMENU | cut -d'^' -f4 | sed "s/#//g"`
LOC=`echo $LOC | sed -e "s/^main//;s/^://;s/:/\\//g"`

if [ ".$PKGINST" = "." ]
then
	PATHNAME="$INTFBASE/$PARENTLOC/$NAME"
else
	PATHNAME="$OAMBASE/add-ons/$PKGINST/$PARENTLOC/$NAME"
fi
PATHNAME=`echo $PATHNAME | sed "s;//;/;g"`
eval OUT=`echo $FILE|sed -e "s/^~/\\$PATHNAME/;s/,~/,\\$PATHNAME/g"`
echo $OUT
