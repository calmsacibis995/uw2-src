#!/usr/bin/winxksh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)pkg.osmp:ifiles/osmp.post.sh	1.25"

#exec 2>/tmp/osmp.err
#set -x

# if this script is being called by the pkgadd of the OSMP package
# done by the boot floppies, then /tmp/osmp.req.sh will still
# exist, and so do nothing.

[ -f /tmp/osmp.req.sh ] && exit 0


PKGMSG=${PKGINST}.pkg
LOCALE=${LC_ALL:-${LC_MESSAGES:-${LANG:-"C"}}}

if [ ! -f /usr/lib/locale/${LOCALE}/LC_MESSAGES/${PKGMSG} ]
then
   if [ -f ${REQDIR}/inst/locale/${LOCALE}/${PKGMSG} -a \
	-d /usr/lib/locale/${LOCALE}/LC_MESSAGES ]
   then
	cp ${REQDIR}/inst/locale/${LOCALE}/${PKGMSG} \
	   /usr/lib/locale/${LOCALE}/LC_MESSAGES
   fi
fi

#
#	User decide continue to try install from floppy
#	or exit now.
#

Psm_Retry ()
{
	case $CHOICE in
	${PsmRetryArray[0]})
		RETRY="TRUE"
		;;
	${PsmRetryArray[1]})
		RETRY="FALSE"
		;;
	esac

	echo "RETRY=$RETRY" >/tmp/retry
}

#
#	check if a true PSM floppy has been inserted.
#

ReadPsmFloppy ()
{

	LOCALE=${LOCALE:-C}
	OSMPLOCALEDIR=/etc/inst/locale/${LOCALE}/menus/osmp
	defaultOSMPLOCALEDIR=/etc/inst/locale/C/menus/osmp
	
	[ -r ${OSMPLOCALEDIR}/txtstrings ] &&
	        . ${OSMPLOCALEDIR}/txtstrings || {
	        #have to use C locale, we have no choice. Used in help
	        OSMPLOCALEDIR=${defaultOSMPLOCALEDIR}
	        . ${defaultOSMPLOCALEDIR}/txtstrings
	}

	integer t
	MENU_TYPE=regular
	PSM_ERROR=0
	export MENU_TYPE PSM_ERROR 

#	eval display -w \"$PSM_REINSERT\"
	place_window $PSM_REINSERTCols $PSM_REINSERTLines+5 -fg $WHITE -bg $BLUE
	wprintf $CURWIN "$PSM_REINSERT"
	footer $PSM_FLOPFOOTER
	call getkey
	wclose $CURWIN

	RETRY="TRUE"
	>/tmp/retry
	while [ "${RETRY}" = "TRUE" ]
	do
		PSM_ERROR=1
		if dd if=/dev/rdsk/f0t bs=512 count=1 >/dev/null 2>&1
		then
			MEDIA=diskette1
			PSM_ERROR=0
		else 
			if dd if=/dev/rdsk/f1t bs=512 count=1 >/dev/null 2>&1
			then
				MEDIA=diskette2
				PSM_ERROR=0
			fi
		fi
		
		[ "${PSM_ERROR}" = "0" ] && {

				pkgadd -p -n -d ${MEDIA} oempsm < /dev/vt02 > /dev/vt02 2>&1
				rc=$?
				call ioctl 0 30213 1 # VT_ACTIVATE to reset to VT 01

				case ${rc} in
				0) PSM_ERROR=0
				   ;;
				10) PSM_ERROR=0
				   ;;
				*) PSM_ERROR=3
				   ;;
				esac
	 	}

	 	case $PSM_ERROR
		 in
			0 ) 
				footer ""
				return ;;
			1 ) # cannot install psm diskette...try again.
				display -w "$PSM_EMSG1" -bg $RED -fg $WHITE 
				footer "$PSM_FLOPFOOTER"
				input_handler
				;;
			2) # inserted wrong psm diskette.
				display -w "$PSM_EMSG2" -bg $RED -fg $WHITE 
				footer "$PSM_FLOPFOOTER"
				input_handler;;
			3) # diskette does not contain a psm package
				display -w "$PSM_EMSG3" -bg $RED -fg $WHITE 
				footer "$PSM_FLOPFOOTER"
				input_handler;;
		esac
		typeset OIFS="$IFS"
		IFS="$nl"
		set -A PsmRetryArray ${PsmRetryList}
		IFS="$OIFS"
		PsmRetry="${PsmRetryArray[0]}"
		typeset CHOOSE_FOOTER="$PsmRetryFooter" 
		typeset CHOOSE_TITLE="$PsmRetryENTRY"
		choose -f -e -exit 'Psm_Retry' -winparms "-fg $COMBO2_FG -bg $COMBO2_BG" "$PsmRetry" "${PsmRetryArray[@]}"
		input_handler
		PSM_ERROR=0
		. /tmp/retry
		[ "${RETRY}" = "FALSE" ] && {
			display -w "$PSM_EMSG4" -bg $RED -fg $WHITE 
			footer "$PSM_FLOPFOOTER"
			input_handler
		}
	done

}

# if this script is being called by the boot floppies right
# before the kernel rebuild, the file /tmp/psmindex
# will still exist.  Need to convert info in the file into
# a value for the variable "DIR"

PSMFLOP=FALSE
CONFIGURED_SOMETHING=FALSE

[ -f /tmp/psmindex ] && {
	. /tmp/psmindex
	case ${PSMINDEX} in
		0) DIR=""
		   OTHERS="compaq pcmp tricord cbus ast olivetti acer"
		   ;;
		1) DIR=compaq
		   OTHERS="pcmp tricord cbus ast olivetti acer"
		   ;;
		2) DIR=pcmp
		   OTHERS="compaq tricord cbus ast olivetti acer"
		   ;;
		3) DIR=tricord
		   OTHERS="compaq pcmp cbus ast olivetti acer"
		   ;;
		4) DIR=cbus
		   OTHERS="compaq pcmp tricord ast olivetti acer"
		   ;;
		5) DIR=ast
		   OTHERS="compaq pcmp tricord cbus olivetti acer"
		   ;;
		6) DIR=olivetti
		   OTHERS="compaq pcmp tricord cbus ast acer"
		   ;;
		7) DIR=acer
		   OTHERS="compaq pcmp tricord cbus ast olivetti"
		   ;;
		8) DIR=""
		   OTHERS="compaq pcmp tricord cbus ast olivetti acer"
		   PSMFLOP=TRUE
	esac
}

FAILURE=1
TMP=/tmp/osmp.err
rm -f ${TMP}

do_install() {
	MOD=$1
	cd /tmp/${MOD}
	/etc/conf/bin/idinstall -k -P osmp -a ${MOD} 2>> ${TMP} ||
	/etc/conf/bin/idinstall -k -P osmp -u ${MOD} 2>> ${TMP} || {
	pfmt -s nostd -g ${PKGMSG}:2 "The installation of the %s package cannot be completed\nbecause of an error in the driver installation.\nThe file %s contains the errors.\n" ${NAME} ${TMP}
	exit ${FAILURE}
	}
	
	CONFIGURED_SOMETHING=TRUE
}

#Install modules, but get list of files to removef before
#doing the idinstall since it deletes the files

if [ -n "$DIR" ] 
then
	for i in ${DIR} ${OTHERS}
	do
	  VOLATILES=`echo /tmp/$i /tmp/${i}/*`
	  removef osmp ${VOLATILES} >/dev/null 1>/dev/null 2>&1
	done
	do_install $DIR
else
	for i in ${OTHERS}
	do
	  VOLATILES=`echo /tmp/$i /tmp/${i}/*`
	  removef osmp ${VOLATILES} >/dev/null 1>/dev/null 2>&1
	done
fi

#
# Is the VxFS Advanced Package installed?
#

VXFS_PACKD=/etc/conf/pack.d/vxfs

pkginfo -i vxfs > /dev/null 2>&1
if [ "$?" = "0" ]
then
	ln ${VXFS_PACKD}/vx_mp.o ${VXFS_PACKD}/Driver_mp.o
	installf osmp ${VXFS_PACKD}/Driver_mp.o=${VXFS_PACKD}/vx_mp.o l
else
	ln ${VXFS_PACKD}/vj_mp.o ${VXFS_PACKD}/Driver_mp.o
	installf osmp ${VXFS_PACKD}/Driver_mp.o=${VXFS_PACKD}/vj_mp.o l
fi

#
# Cleanup
#

cd /
rm -rf ${VOLATILES} 1>/dev/null 2>&1

removef -f osmp >/dev/null
installf -f osmp >/dev/null

if [ "${CONFIGURED_SOMETHING}" = "TRUE" ]
then
	# always call idtype to set /etc/conf/cf.d/type if a PSM
	# was configured
	/etc/conf/bin/idtype mp 1>/dev/null 2>/dev/null

	# if /etc/inst/scripts/pdiconfig doesn't exist, means 
	# this is a non-boot flop installation of the package,
	# so a kernel build is in order. Exit 10 so
	# the need to reboot is flagged

	[ ! -f /etc/inst/scripts/pdiconfig ] && {
		/etc/conf/bin/idbuild 2>${TMP}
		[ "$?" != 0 ] && exit ${FAILURE}
		
	}
fi


# We do this if being called during boot floppy installation.
# Check for PSMFLOP=TRUE, and call winxksh functions to
# prompt for insertion of the PSM floppy.

if [ "${PSMFLOP}" = "TRUE" ]
then
	ReadPsmFloppy
fi


