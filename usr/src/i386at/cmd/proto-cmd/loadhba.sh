#!/usr/bin/xksh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto-cmd:i386at/cmd/proto-cmd/loadhba.sh	1.16"

function func_hbainst
{
	typeset HBA_ROOTDIR=$1

	cat ${HBA_ROOTDIR}/etc/modules >/tmp/ihvhba.list.$$
	/sbin/modadmin -s | cut -d: -f3 >/tmp/modloaded.$$
	for driver in `cat /tmp/ihvhba.list.$$`
	do
		grep "/$driver$" /tmp/modloaded.$$ >/dev/null 2>&1
		if [ $? -eq 0 ]
		then
				echo $driver >> /tmp/hbas.loaded.$$
		fi
	done
	rm -f /tmp/modloaded.$$ >/dev/null 2>&1
	rm -f /tmp/ihvhba.list.$$ >/dev/null 2>&1
	if [ -f /tmp/hbas.loaded.$$ ]
	then
	
		for hba in `cat /tmp/hbas.loaded.$$`
		do
			for pkg in `cat /tmp/ihvname.$$`
			do
				grep $hba ${HBA_ROOTDIR}/$pkg/pkgmap >/dev/null 2>&1
				if [ $? -eq 0 ]
				then
					echo "$pkg" >>/tmp/pkg.add.$$    
					break;
				fi
			done
		done	
	fi
	rm -f /tmp/hbas.loaded.$$
	PKGARG=""
	[ -f /tmp/pkg.add.$$ ] && {
	
		for i in `cat /tmp/pkg.add.$$`
		do
			PKG=$i
			PKGARG="$PKGARG $PKG"
		done
		PKGS=$PKGARG
		rm -f /tmp/pkg.add.$$ >/dev/null 2>&1
	}
}

function ReadHbaFloppy
{

[ -n "$SH_VERBOSE" ] && set -x
integer t m=0
typeset MEDIA=diskette1
typeset ishba
HBA_NAME="$1"
MENU_TYPE=regular
HBA_ERROR=0
export MENU_TYPE

place_window $HBA_REINSERTCols $HBA_REINSERTLines+5 -fg $WHITE -bg $BLUE
wprintf $CURWIN "$HBA_REINSERT"
t=${#HBA_PROMPT}/2
wgotoxy $CURWIN $HBA_REINSERTCols/2-$t  $HBA_REINSERTLines+1
wprintf $CURWIN "$HBA_PROMPT\n"
footer $HBA_FOOTER
call getkey
wclose $CURWIN

eval footer "\"$HBA_LOAD_FOOTER\""

while :
do
	mntsts="1"
	ishba="1"
	/sbin/mount -Fs5 -r /dev/dsk/f0t /install 2>/dev/null || /sbin/mount -Fs5 -r /dev/dsk/f1t /install 2>/dev/null
	mntsts="$?"
	hbaformat="1"

	if [ "$mntsts" = "0" ] 
	then
		loadname=""
		if [ -f /install/etc/loadmods -a -f /install/etc/load.name ]
		then
			hbaformat="0"
			grep -v '^$' /install/etc/load.name > /tmp/loadname
			read loadname < /tmp/loadname
			if [ "$HBA_NAME" = "$loadname" ]
			then
				ishba="0"
				[ "${FLPYDEV}" = /dev/dsk/f1t ] && MEDIA=diskette2
			fi

			HERE=`pwd`
			cd /install
			> /tmp/ihvname.$$
			for i in *
			do
				[ -f $i/pkgmap ] && {
					echo $i >> /tmp/ihvname.$$
				}
			done
			cd ${HERE}
		fi

		PKGS=""
		if [ -f /install/etc/HBAINST ]
		then
			# UW1.1 compatibiliy
			PKGS="`. /install/etc/HBAINST`"
		else
			# Intersection of modules loaded and related
			# packages on the IHV HBA floppy OR "all" if
			# intersection doesn't exist.
			func_hbainst "/install"
			[ "$PKGS" = "" ] && PKGS=all
		fi

		/sbin/umount /install	 2>/dev/null
		if [ "$ishba" = "0" -a -n "$PKGS" ] 
		then
			HBA_ERROR=1

			#
			# We only want to run pdiadd from the HBA package's
			# postinstall script if the package is being added
			# as an add-on (not from this script).   Provide a
			# means for determining via a value in SETNAME
			# package environment variable.
			#
			SETNAME=from_loadhba /usr/sbin/pkgadd -p -l -q -d ${MEDIA} $PKGS  < /dev/tty
			rc=$?
			[ "${rc}" = "0" -a -f /tmp/ihvname.$$ ] && {
				rm -f /tmp/ihvname.$$ 1>/dev/null 2>&1
				HBA_ERROR=0
			}
		else
		   if [ -z "$PKGS" ]
		   then
			HBA_ERROR=4
		   elif [ "$hbaformat" = "0" ] 
		   then
			HBA_ERROR=2
		   else
			HBA_ERROR=3
		   fi
		fi
	else
		HBA_ERROR=1
	fi
	case $HBA_ERROR
	in
	0 ) 	footer ""	#clear footer on exit
		return ;;
	1 ) # cannot install hba diskette...try again.
		display -w "$HBA_EMSG1" -bg $ERROR_BG -fg $ERROR_FG
		input_handler;;
	2) # inserted wrong hba diskette.
		display -w "$HBA_EMSG2" -bg $ERROR_BG -fg $ERROR_FG
		input_handler;;
	3) # diskette is not an hba diskette.
		display -w "$HBA_EMSG3" -bg $ERROR_BG -fg $ERROR_FG
		input_handler;;
	4) # no drivers on hba diskette are needed on system
		display -w "$HBA_EMSG4" -bg $ERROR_BG -fg $ERROR_FG
		input_handler;;
	esac
done
}

function ReadHbaCD
{
	typeset CDROOT="/cd-rom/.hba.flop"
	typeset ishba="1"
	typeset loadname=""

	[ "$SEC_MEDIUM_TYPE" != "cdrom" ] && return 1

	if [ -f ${CDROOT}/etc/loadmods -a -f ${CDROOT}/etc/load.name ]
	then
		grep -v '^$' ${CDROOT}/etc/load.name | read loadname
		[ "$1" = "$loadname" ] && ishba="0"

		HERE=`pwd`
		cd ${CDROOT}
		> /tmp/ihvname.$$
		for i in *
		do
			[ -f $i/pkgmap ] && {
				echo $i >> /tmp/ihvname.$$
			}
		done
		cd ${HERE}
	fi

	PKGS=""
	if [ -f ${CDROOT}/etc/HBAINST ]
	then
		# UW1.1 compatibiliy
		PKGS="`. ${CDROOT}/etc/HBAINST`"
	else
		func_hbainst "$CDROOT"
		[ "$PKGS" = "" ] && PKGS=all
	fi

	if [ "$ishba" = "0" -a -n "$PKGS" ] 
	then
		SETNAME=from_loadhba /usr/sbin/pkgadd -p -l -q -d "$CDROOT" $PKGS </dev/tty
		rc=$?
		[ "${rc}" = "0" -a -f /tmp/ihvname.$$ ] && {
			for i in `cat /tmp/ihvname.$$`
			do
			   create_reverse_depend $i
			done
			rm -f /tmp/ihvname.$$ 1>/dev/null 2>&1
		}
	fi

	HBA_ERROR=0
	footer ""
	return 0
}

integer n=1
typeset -x HBA_PROMPT
export HBA_ERROR HBA_PROMPT

[ ! -d /install ] && {
mkdir /install
}
[ ! -d /flpy2 ] && {
	sh_umount /flpy2	2>/dev/null
}

[ -f /etc/inst/scripts/oem.sh ] && /etc/inst/scripts/oem.sh

while [ "${IHVHBAS[n]}" != END ]
do
	let n+=1
done

while (( n-=1 ))
do
	if [ -n "${IHVHBAS[n]}" ]
	then
		HBA_ERROR=1
		while [ "$HBA_ERROR" != 0 ]
		do
			HBA_PROMPT="${IHVHBAS[n]}"
			case "${IHVHBAMEDIA[n]}" in
			diskette)
				ReadHbaFloppy "${IHVHBAS[n]}"
				;;
			cdrom)
				ReadHbaCD "${IHVHBAS[n]}"
				;;
			*)
				;;
			esac
		done
	fi
done
unset -f ReadHbaFloppy
unset -f ReadHbaCD

