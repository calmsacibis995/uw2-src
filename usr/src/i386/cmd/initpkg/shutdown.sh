#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)initpkg:i386/cmd/initpkg/shutdown.sh	1.8.19.6"
#ident "$Header: shutdown.sh 1.4 91/07/08 $"

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
	# if catalogs aren't under /usr/lib/locale, check /etc/inst/locale
	if [ -d /usr/lib/locale/$LANG ] 
	then LC_MESSAGES=$LANG
	else LC_MESSAGES=/etc/inst/locale/$LANG
	fi
	export LANG LC_MESSAGES
fi
LABEL="UX:shutdown"
CAT="uxshutdown"

#	Sequence performed to change the init state of a machine.

#	This procedure checks to see if you are permitted and allows an
#	interactive shutdown.  The actual change of state, killing of
#	processes and such are performed by the new init state, say 0,
#	and its /sbin/rc0.

#	Usage:  shutdown [ -y ] [ -g<grace-period> ] [ -i<init-state> ]

priv -allprivs work	# turn off all working privileges

if [ `pwd` != / ]
then
	pfmt -l $LABEL -s error -g $CAT:4 "You must be in the / directory to run /sbin/shutdown.\n"
	exit 1
fi

# Make sure /usr is mounted
if [ ! -d /usr/bin ]
then
	pfmt -l $LABEL -s error -g $CAT:5 "/usr is not mounted.  Mount /usr or use init to shutdown.\n"
	exit 1
fi

askconfirmation=yes

if i386
then
	initstate=0
else
	initstate=s
fi

while getopts ?yg:i: c
do
	case $c in
	i)	initstate=$OPTARG; 
		case $initstate in
		2|3|4)
			pfmt -l $LABEL -s error -g $CAT:8 "Initstate %s is not for system shutdown.\n" $initstate
			exit 1
		esac
		;;
	g)	grace=$OPTARG; 
		;;
	y)	askconfirmation=
		;;
	\?)	pfmt -l $LABEL -s action -g $CAT:9 "Usage:  %s [ -y ] [ -g<grace> ] [ -i<initstate> ]\n" $0
		exit 2
		;;
	*) 	
		pfmt -l $LABEL -s action -g $CAT:9 "Usage:  %s [ -y ] [ -g<grace> ] [ -i<initstate> ]\n" $0
		exit 2
		;;
	esac
done
if [ -z "$grace" ]
then
	if [ -r /etc/default/shutdown ]
	then
		grace=`grep grace /etc/default/shutdown`
		if [ "${grace}" = "" ]
		then
			pfmt -l $LABEL -s warn -g $CAT:6 "Could not read /etc/default/shutdown.\n"
			pfmt -s nostd -g $CAT:7 "\tSetting default grace period to 60 seconds.\n"
			grace=60
		else
			eval $grace
		fi
	else
		pfmt -l $LABEL -s warn -g $CAT:6 "Could not read /etc/default/shutdown.\n"
		pfmt -s nostd -g $CAT:7 "\tSetting default grace period to 60 seconds.\n"
		grace=60
	fi
fi

shift `expr $OPTIND - 1`

if [ -x /usr/alarm/bin/event ]
then
	/usr/alarm/bin/event -c gen -e shutdown -- -t $grace
fi

if [ -z "${TZ}"  -a  -r /etc/TIMEZONE ]
then
	. /etc/TIMEZONE
fi

pfmt -l $LABEL -s info -g $CAT:10 '\nShutdown started.    ' 2>&1
/usr/bin/date
echo

/sbin/sync&

trap "exit 1"  1 2 15

WALL()
{
	SAVE_LC=$LC_ALL
	LC_ALL=`defadm locale LANG | cut -d= -f2` 
	export LC_ALL
	pfmt -l UX:shutdown -s warn "$@" 2>&1 | /usr/sbin/wall
	LC_ALL=$SAVE_LC
}

set -- `/sbin/who`
if [ $? -eq 0 -a $# -gt 5  -a  ${grace} -gt 0 ]
then
	priv +macwrite +dacwrite +dev work
	WALL -g uxshutdown:1 "The system will be shut down in %s seconds.\nPlease log off now.\n\n" ${grace}
	priv -allprivs work
	/usr/bin/sleep ${grace} 2>/dev/null
	if [ $? -eq 2 ]
	then
		pfmt -l $LABEL -s error -g $CAT:11 "Invalid grace period provided.\n"
		pfmt -l $LABEL -s action -g $CAT:9 "Usage:  %s [ -y ] [ -g<grace> ] [ -i<initstate> ]\n" $0
		exit 2
	fi
	priv +macwrite +dacwrite +dev work

	WALL -g uxshutdown:2 "THE SYSTEM IS BEING SHUT DOWN NOW ! ! !\nLog off now or risk your files being damaged.\n\n"
	priv -allprivs work
fi	

if [ ${grace} -ne 0 ]
then
	/usr/bin/sleep ${grace} 2>/dev/null
fi
if [ $? -eq 2 ]
then
	pfmt -l $LABEL -s error -g $CAT:11 "Invalid grace period provided.\n"
	pfmt -l $LABEL -s action -g $CAT:9 "Usage:  %s [ -y ] [ -g<grace> ] [ -i<initstate> ]\n" $0
	exit 2
fi

y=`gettxt $CAT:12 "y"`
n=`gettxt $CAT:13 "n"`
if [ "${askconfirmation}" ]
then
	pfmt -s nostd -g $CAT:14 "Do you want to continue? (%s or %s):   " $y $n 2>&1
	read b
else
	b=$y
fi
if [ "$b" != "$y" ]
then
	priv +macwrite +dacwrite +dev work
	WALL -g uxshutdown:3 "False Alarm:  The system will not be brought down.\n"
	priv -allprivs work
	pfmt -l $LABEL -s info -g $CAT:15 'Shut down aborted.\n'
	exit 1
fi
case "${initstate}" in
s | S )
	priv +allprivs work
	. /sbin/rc0 shutdown_s
	priv -allprivs work
esac

RET=0
priv +owner +compat work
/sbin/init ${initstate}
RET=$?
priv -allprivs work
exit ${RET}
