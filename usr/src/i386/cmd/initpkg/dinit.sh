#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)initpkg:i386/cmd/initpkg/dinit.sh	1.2.1.7"

# 	Startup scripts that can be delayed to be run after login
#	processes have been started. Should run fast!

#INFO messages should not appear in screen, save them in /var/adm/dinit.log
exec >/var/adm/dinit.log
chmod 664 /var/adm/dinit.log

if [ -z "$LC_ALL" -a -z "$LC_MESSAGES" ]
then
	if [ -z "$LANG" ]
	then
		LNG=`defadm locale LANG 2>/dev/null`
		if [ "$?" != 0 ]
		then LANG=C
		else eval $LNG
		fi
	fi
	LC_MESSAGES=/etc/inst/locale/$LANG
	export LANG LC_MESSAGES
fi
LABEL="UX:$0"

CAT=uxrc; export CAT

set -- `LC_ALL=C /sbin/who -r`
_CURR_RL=$7 _CURR_NTIMES=$8 _PREV_RL=$9 export _CURR_RL _CURR_NTIMES _PREV_RL
unset _AUTOBOOT _AUTOKILL _AUTOUMOUNT
case "$_PREV_RL" in
s | S )
	case "$_CURR_RL" in
	2 | 3 )
		if [ "$_CURR_NTIMES" -eq 0 ]
		then
			_AUTOBOOT=true export _AUTOBOOT
		fi
		;;
	esac
	:
	cd /etc/dinit.d 2>/dev/null
	if [ $? -eq 0 ]
	then
		for f in K*
		{
			case $f in
			K\* ) ;;
			* ) /sbin/sh ${f} stop ;;
			esac
		}

		for f in S*
		{
			case $f in
			S\* ) ;;
			* ) /sbin/sh ${f} start ;;
			esac
		}
	fi
	;;
esac
