#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)maplang:maplang.sh	1.1"

#
# maplang [ -l lang ] [ -d dir ] [ -f file ]
#
# Maps language+territory locale to appropriate language.
# Checks installation of locale in dir (default /etc/inst/locale)
# Backups to the first available installed language
#
LNG="${LC_ALL:-$LC_MESSAGES}"
TESTDIR="/etc/inst/locale"
TESTFILE="menus"
while [ $# -ge 2 ]
do
	case "$1" in
	-l) LNG="$2" ;;
	-d) TESTDIR="$2" ;;
	-f) TESTFILE="$2" ;;
	*) : ;;
	esac
	shift 2
done
[ "$LNG" ] || LNG=$LANG
[ "$LNG" ] || LNG=C
MAP_LNG=`expr "$LNG" : '\([^_]*\).*'`
case "$MAP_LNG" in
   C)     DEF="C" ;;
   da)    DEF="da:en:C" ;;
   de)    DEF="de:en:C" ;;
   en)    DEF="en:C" ;;
   es)    DEF="es:en:C" ;;
   "fi")  DEF="fi:en:C" ;;
   fr)    DEF="fr:en:C" ;;
   is)    DEF="is:en:C" ;;
   it)    DEF="it:en:C" ;;
   nl)    DEF="nl:en:C" ;;
   no)    DEF="no:en:C" ;;
   pt)    DEF="pt:es:en:C" ;;
   sv)    DEF="sv:en:C" ;;
   *)	  DEF="en:C" ;;
esac
while
	TEST_LNG=`expr "$DEF" : '\([^:]*\).*'`
	DEF=`expr "$DEF" : '[^:]*:\(.*\)'`
	[ "$TEST_LNG" ]
do
	if [ -f "$TESTDIR/$TEST_LNG/$TESTFILE" ]
	then
		echo $TEST_LNG
		exit 0
	fi
done
echo "C"
exit 0
