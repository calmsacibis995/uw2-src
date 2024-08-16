#!/usr/bin/winxksh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)pkg.osmp:ifiles/osmp.req.sh	1.20"

#exec 2>/tmp/osmp.err
#set -x
LOCALE=${LC_ALL:-${LC_MESSAGES:-${LANG:-"C"}}}

PSM_set ()
{
	PSM=$CHOICE
	echo "PSM=${PSM}" > /tmp/psm
}

choose_PSM ()
{
	typeset CHOOSE_FOOTER="$PSM_RFOOTER" CHOOSE_TITLE="$PSM_ENTRY"

	choose -e -exit 'PSM_set' -winparms "-fg $COMBO2_FG -bg $COMBO2_BG" "$PSM" "${PSMARRAY[@]}"
	input_handler
	. /tmp/psm
	index=0
	while [ 1 ]
	do
		[ "${PSMARRAY[${index}]}" = "${PSM}" ] && break
		let index=index+1
	done
	echo "PSMINDEX=${index}" > /tmp/psmindex
}

select_PSM ()
{
	if [ -z "${PSMARRAY}" ]
	then
		typeset OIFS="$IFS"
		IFS="$nl"
		if [ "${ON_BOOTS}" = "FALSE" ]
		then
			set -A PSMARRAY ${PSMLIST0}
		else
			set -A PSMARRAY ${PSMLIST1}
		fi
		IFS="$OIFS"
	fi

	PSMINDEX=0
	case ${DIR} in

		ast) PSMINDEX=5;;
		compaq) PSMINDEX=1;;
		tricord) PSMINDEX=3;;
		cbus) PSMINDEX=4;;
		pcmp) PSMINDEX=2;;
		olivetti) PSMINDEX=6;;
		acer) PSMINDEX=7;;

	esac

	PSM="${PSMARRAY[${PSMINDEX}]}"
	echo "PSM=${PSM}" > /tmp/psm
	echo "PSMINDEX=${PSMINDEX}" > /tmp/psmindex
	export PSM PSMINDEX
	place_window ${PSM_WIDTH}+5 ${PSM_DEPTH} -title "$PSM_ENTRY" 
	typeset wid=$CURWIN
	set_hotkey 5 choose_PSM
	if [ "${PSMINDEX}" = "0" ]
	then
	   wprintf $wid "${NOPSM_MSG}" 
	else
	   wprintf $wid "%s %s %s ${PSM_MSG1} ${PSM} ${PSM_MSG2}"
	fi
	footer "${PSM_FOOTER}"
	call getkey
	wclose $wid
	footer ""
}

############# Begin UPGRADE AND OVERLAY #######################

# Don't need to worry about UPGRADE case for UW2.0 since the OSMP
# is new in UW2.0

SCRIPTS=/usr/sbin/pkginst
. ${SCRIPTS}/updebug

[ "$UPDEBUG" = YES ] && set -x

export PKGINSTALL_TYPE 

PKGINSTALL_TYPE=NEWINSTALL

[ "$UPDEBUG" = YES ] && goany

# the comment below from LP package; I'm big-time confused as this
# command indicates that an SVR4.2 package returns "2"; should't
# a 4.2MP package being placed on a 4.2 package be considered
# an upgrade?

#chkpkgrel, returns a code, indicating which version of this pkg is installed.
#Return code 2 indicates overlay of the same or older version. For overlay,
#existence of the file $UPGRADE_STORE/$PKGINST.ver indicates presence of older
#version. This file contains the old version.

#	${SCRIPTS}/chkpkgrel returns    0 if pkg is not installed
#					1 if pkg if unknown version
#					2 if pkg is SVR4.2
#					4 if pkg is SVR4.0 V4
#					9 if newer pkg is installed

${SCRIPTS}/chkpkgrel osmp
PKGVERSION=$?

case $PKGVERSION in
	2)	PKGINSTALL_TYPE=OVERLAY	;;
	9)	exit 3	;; #pkgrm newer pkg before older pkg can be installed.
	*)	;;
esac

[ "$UPDEBUG" = YES ] && goany

[ "${PKGINSTALL_TYPE}" = "OVERLAY" ] && {

	# restore type to "atup" in case install fails; postinstall
	# of this package will restore it to "mp"
	/etc/conf/bin/idtype atup 1>/dev/null 2>&1

	[ "$UPDEBUG" = YES ] && {
		echo "about to check on oempsm package"
		goany
	}

	# now remove the "oempsm" package, if it exists
	pkginfo -i oempsm 1>/dev/null 2>&1
	rc=$?
	[ "${rc}" = "0" ] && {
		pkgrm -n oempsm 1>/dev/null 2>&1
		rc=$?
		[ "$UPDEBUG" = YES ] && {
		  echo "return code from pkgrm of oempsm was " ${rc}
		  goany
		}
	}

	#now check to see whether a PSM installed by OSMP is already
	#configured

	for i in ast pcmp cbus compaq tricord olivetti acer
	do
		/etc/conf/bin/idcheck -p $i
		rc=$?
		[ "$UPDEBUG" = YES ] && {
		  echo "idcheck of " $i " returned " ${rc}
		  goany
		}

		[ "${rc}" != "0" -a "${rc}" != "100" ] && {
		  # driver was configured;  remove it if sdevice contains
		  # $interface psm (watch out for silly name conflict)

		  if grep interface /etc/conf/mdevice.d/$i | grep psm
		  then
			/etc/conf/bin/idinstall -P osmp -d $i
		  fi 1>/dev/null 2>&1
		}
	done

	# now remove /etc/conf/pack.d/vxfs/Driver.o_mp so that
	# code in postinstall doesn't complain about vxfs ln's
	rm -f /etc/conf/pack.d/vxfs/Driver_mp.o 1>/dev/null 2>&1
	
}

[ "$UPDEBUG" = YES ] && goany

############# End  UPGRADE AND OVERLAY #######################

# detect if being invoked via boot floppy!

if [ -f /etc/inst/scripts/pdiconfig ]
then
	ON_BOOTS=TRUE
else
	ON_BOOTS=FALSE
fi

# For DEBUGGING!
# echo ON_BOOTS is $ON_BOOTS
#

# Call autodetect to install appropriate modules.
unset DIR OTHERS
/usr/bin/autodetect 1>/dev/null 2>&1
case $? in
	101|102 )
		DIR=cbus
		OTHERS="ast pcmp compaq tricord olivetti acer"
		;;
	1 )
		DIR=compaq
		OTHERS="ast pcmp cbus tricord olivetti acer"
		;;
	2 )
		DIR=ast
		OTHERS="pcmp cbus compaq tricord olivetti acer"
		;;

	3 )
		DIR=acer
		OTHERS="pcmp cbus compaq tricord olivetti"
		;;

	4 )
		DIR=tricord
		OTHERS="ast pcmp cbus compaq olivetti acer" 
		;;
	6 | 7 )
		DIR=pcmp
		OTHERS="ast cbus compaq tricord olivetti acer"
		;;
	8 )
		DIR=olivetti
		OTHERS="ast pcmp cbus compaq tricord acer" 
		;;
	*)
		OTHERS="ast pcmp cbus compaq tricord olivetti acer"
		;;
esac

#
#	Not called from boot floppy, 
#	so set up winxksh environment.
#
if [ "${ON_BOOTS}" = "FALSE" ] 
then
	. /tmp/osmp_wininit
else

	LOCALE=${LOCALE:-C}
	OSMPLOCALEDIR=/etc/inst/locale/${LOCALE}/menus/osmp
	defaultOSMPLOCALEDIR=/etc/inst/locale/C/menus/osmp
	
	[ -r ${OSMPLOCALEDIR}/txtstrings ] &&
	        . ${OSMPLOCALEDIR}/txtstrings || {
	        #have to use C locale, we have no choice. Used in help
	        OSMPLOCALEDIR=${defaultOSMPLOCALEDIR}
	        . ${defaultOSMPLOCALEDIR}/txtstrings
	}
fi

# silent mode boot floppy installation support
# check if ${SILENT_INSTALL} = true.  Set PSMINDEX to "1" (Compaq)

if [ "${ON_BOOTS}" = "TRUE" ] && ${SILENT_INSTALL} && [ "$PLATFORM" = "compaq" ]
then
	if [ "$DIR" = "pcmp" ]
	then
		PSMINDEX=2
	else
		PSMINDEX=1
	fi
	echo "PSMINDEX=$PSMINDEX" > /tmp/psmindex
else

	#
	# Just
	#	1. prompt the user for PSM selection,
	#	2. store her/his choice under #	/tmp/psm and /tmp/psmindex, and
	#       3. Boot floppies will (1) execute /tmp/osmp.req.sh
	#                             (2) rm /tmp/osmp.req.sh # IMPORTANT!!!!
	#                             (3) execute /tmp/osmp.post.sh 
	
	select_PSM
fi

#
#	Reset tty and clean up the screen 
#
[ "${ON_BOOTS}" = "FALSE" ]  && . /tmp/osmp_winexit

