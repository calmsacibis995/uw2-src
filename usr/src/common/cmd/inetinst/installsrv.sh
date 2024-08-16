#!/bin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)inetinst:installsrv.sh	1.18"

#
#  Installsrv
#
#  Command-line interface for setting up an Install Server
#

#
#  Set up global variables
#
DFLT_PRODUCT=XYZXYZ
CMD_NAME=`basename $0`
SPOOL_BASE="/var/spool/dist"
MOUNTDIR="${SPOOL_BASE}/mount"
ADM_BASE="/var/sadm/dist"
SERVICE="inetinst"
LOCALES="C de es fr it ja"
SPX_PORT="1006"
SPX_PORT_HEX="0x3ee"
SPX_SERVICE="1006"
SPX_SERVICE_HEX="0x3ee"
UW_SERVICE_HEX="0x3e4"
MY_HOSTNAME=`/bin/uname -n`

#
#  initialize message strings
#

LANG=${LC_ALL:-${LC_MESSAGES:-${LANG}}}
if [ -z "$LANG" ]
then
    LNG=`defadm locale LANG 2>/dev/null`
    if [ "$?" != 0 ]
    then
	LANG=C
	else eval $LNG
    fi
    export LANG
fi

GETTXT="eval /usr/bin/gettxt"
PFMT="eval /bin/pfmt -l UX:installsrv"

#
#	other text
#
MSG_CMOUNT='installsrv:1	 "Could not mount %s on %s\n"'
MSG_CSPOOL='installsrv:2	 "Could not spool %s.package\n"'
MSG_CLINK='installsrv:3	 "Could not link %s.package\n"'
MSG_CCOPY='installsrv:4	 "Could not copy %s.package from %s\n"'
MSG_NOSPACE='installsrv:5	 "Not enough space on disk\n"'
MSG_ENARG='installsrv:6	 "enable: bad arg passed (%s): must be tcp or spx.\n"'
MSG_DISARG='installsrv:7	 "disable: bad arg passed (%s): must be tcp or spx.\n"'
MSG_ISARG='installsrv:8	 "isthere: bad arg passed (%s): must be tcp or spx.\n"'
MSG_ENABLE='installsrv:9	 "%s networking is enabled\n"'
MSG_SENABLED='installsrv:10	 "Service %s enabled for %s\n"'

MSG_U1='installsrv:11	 "Usage:\n"'
MSG_U2='installsrv:12	 "\t$CMD_NAME -e|u [-n spx|tcp]\n"'
MSG_U3='installsrv:13	 "\t$CMD_NAME -q\n"'
MSG_U4='installsrv:14	 "\t$CMD_NAME -l as|pe -d device -c spool|mount [-p product_name] [-L locale]\n"'
MSG_U5='installsrv:15	 "\t$CMD_NAME -l as|pe -s servername [-p product_name] [-L locale]\n"'

MSG_NODEV='installsrv:16	 "Invalid Device %s Specified.\n"'
MSG_NOOPT='installsrv:17	 "No options specified.\n"'
MSG_BADQ='installsrv:18	 "Cannot specify -q with any other options.\n"'
MSG_ENUN='installsrv:19	 "Enable (-e) and disable (-u) mutually exclusive\n"'
MSG_BADN='installsrv:20	 "If network (-n) specified, must either enable or disable\n"'
MSG_DS='installsrv:21	 "Device (-d) and server (-s) mututally exclusive\n"'
MSG_CD='installsrv:22	 "Copy (-c) must be specified if device (-d) used\n"'
MSG_NMOUNT='installsrv:23	 "Cannot mount from a remote server\n"'
MSG_NOTCD='installsrv:24	 "Can only mount from a CD-ROM device.\n"'
MSG_NLOCALE='installsrv:25	 "Must specify a locale (-L) or LANG must be set in environment\n"'
MSG_NPROD='installsrv:26	 "Must specify a product (-p)\n"'
MSG_NCOPY='installsrv:27	 "main: COPY not set correctly: %s\n"'

TXT_LOCALE='installsrv:28	"Locale:"'
TXT_AS='installsrv:29	"Application Server"'
TXT_PE='installsrv:30	"Personal Edition"'



#
#  Mount a cdrom
#
mountcd()
{
	#
	#  don't do anything if already mounted
	#
	MountData=`/usr/sbin/mount | grep ${BDEV}`
	if [ "$MountData" ]
	then
	    set $MountData
	    MOUNTDIR="$1"
	    PREMOUNT=":"
	else
	    #
	    #  Make the mounting directory if it is not currently there
	    #
	    [ ! -d "${MOUNTDIR}" ] && /bin/mkdir -p ${MOUNTDIR} >/dev/null 2>&1

	    #
	    #  Perform the mount operation
	    #
	    /usr/sbin/mount -F cdfs -r ${BDEV} ${MOUNTDIR}
	    [ $? != 0 ] && {
		    $PFMT -g $MSG_CMOUNT ${BDEV} ${MOUNTDIR}
		    exit 19		# ENODEV
	    }
	fi
}


#
#  Spool a package from a device
#
spoolpkg()
{
	/bin/mkdir -p ${SPOOL_BASE}/${PRODUCT}/${LOCALE} > /dev/null 2>&1

	case "${DEVTYPE}" in
	qtape)
		DEV_NODE=`devattr ${DEVICE} cdevice`
		ERROR=0
		for PKG in ${LOAD}
		do
		   dd if=${DEV_NODE} of=${SPOOL_BASE}/${PRODUCT}/${LOCALE}/${PKG}.package bs=256k
		   [ $? != 0 ] && {
		   	ERROR=1
			$PFMT -g $MSG_CSPOOL $PKG
		   }
		done
		[ ${ERROR} != 0 ] && exit 2	# ENOENT
		return 0
		;;
	cdrom)
		MOUNTDIR=`devattr ${DEVICE} mountpt`
		BDEV=`devattr ${DEVICE} bdevice`
		mountcd
		ERROR=0
		for PKG in ${LOAD}
		do
		   cp ${MOUNTDIR}/${PKG}.image ${SPOOL_BASE}/${PRODUCT}/${LOCALE}/${PKG}.package
		   [ $? != 0 ] && {
		   	ERROR=1
			$PFMT -g $MSG_CSPOOL $PKG
		   }
		done
		[ "$PREMOUNT" ] || /usr/sbin/umount ${MOUNTDIR}
		[ ${ERROR} != 0 ] && exit 2	# ENOENT
		return 0
		;;
	esac
}

#
#  Spool a package from a device
#
mntpkg()
{
	mountcd
	ERROR=0

	/bin/mkdir -p ${SPOOL_BASE}/${PRODUCT}/${LOCALE} > /dev/null 2>&1

	#
	#  Make our symbolic links.
	#
	for PKG in ${LOAD}
	do
		ln -s ${MOUNTDIR}/${PKG}.image ${SPOOL_BASE}/${PRODUCT}/${LOCALE}/${PKG}.package
		[ $? = 0 ] || {
			ERROR=1
			$PFMT -g $MSG_CLINK ${PKG}
		}
	done
	[ ${ERROR} != 0 ] && exit 90		# ELOOP
	return 0
}


#
#  Copy a package from another install server
#
copypkg()
{
	/bin/mkdir -p ${SPOOL_BASE}/${PRODUCT}/${LOCALE} > /dev/null 2>&1

	ERROR=0

	for PKG in ${LOAD}
	do
		/usr/sbin/pkgcat -s ${SERVER}:${SPOOL_BASE}/${PRODUCT}/${LOCALE} \
			${PKG}.package > \
			${SPOOL_BASE}/${PRODUCT}/${LOCALE}/${PKG}.package \
			2>/dev/null
		[ $? != 0 ] && {
			$PFMT -g $MSG_CCOPY ${PKG} ${SERVER}
			ERROR=1
		}
	done

	[ ${ERROR} != 0 ] && exit 2		# ENOENT
	return 0
}


#
#  Make sure we have enough space on disk for the stuff we want
#	Returns 0 if there IS enough space 
#	Returns 1 if there's NOT enough space 
#
chkspace()
{
	KBNEEDED=0

	if [ "${COPY}" = "mount" ]
	then
		KBNEEDED=10
	else
		for PKG in ${LOAD}
		do
			[ "${PKG}" = "as" ] && KBNEEDED=`expr KBNEEDED + 60000`
			[ "${PKG}" = "pe" ] && KBNEEDED=`expr KBNEEDED + 40000`
		done
	fi

	KBFREE=`df -k ${SPOOL_BASE} | awk '{print $4}' | tail -1`
	if [ $KBFREE -lt $KBNEEDED ]
	then
		$PFMT -g $MSG_NOSPACE
		exit 28		# ENOSPC
	fi
}

#
#  Enable SPX or TCP interface
#
enable()
{
	case "$1" in
	tcp)
		#
		#  Add entry to /etc/inet/inetd.conf, if not there yet.
		#
		grep "^${SERVICE}" /etc/inet/inetd.conf > /dev/null 2>&1
		[ $? -ne 0 ] && {
		cat >> /etc/inet/inetd.conf <<EOF
inetinst	stream	tcp	nowait	nobody	/usr/sbin/in.inetinst	in.inetinst
EOF
	}

		#
		#  Give the inetd a kick to use new configuration
		#
		INETPID=`ps -luroot | grep " inetd$" | awk '{print $4}'`
		[ ! -z "${INETPID}" ] && kill -HUP ${INETPID}
		return 0
		;;
	spx)
		#
		#  Add entry to /etc/nwnetd.conf, if not there yet.
		#
		grep "^${SERVICE}" /etc/nwnetd.conf > /dev/null 2>&1
		[ $? -ne 0 ] && {
		cat >> /etc/nwnetd.conf <<EOF
inetinst	stream	spx	nowait	nobody	/usr/sbin/in.inetinst	in.inetinst
EOF
		}

		#
		#  Give the nwnetd a kick to use new configuration
		#
		NWNETPID=`ps -luroot | grep " nwnetd$" | awk '{print $4}'`
		[ ! -z "${NWNETPID}" ] && kill -HUP ${NWNETPID}

		#
		#  If this is a UnixWare 1.0/1.1 machine,
		#  give the sapd a kick to use new configuration
		#
		if [ -f /usr/sbin/nwsaputil ]
		then
			/usr/sbin/nwsaputil -a -t ${SPX_SERVICE_HEX} -n ${MY_HOSTNAME} -s ${SPX_PORT_HEX}
			return $?
		else if [ -f /var/spool/sap/out/${UW_SERVICE_HEX} ]
		  then
			echo ${SPX_PORT} > /var/spool/sap/out/${SPX_SERVICE_HEX}
			SAPDPID=`ps -luroot | grep " sapd$" | awk '{print $4}'`
			[ ! -z "${SAPDPID}" ] && kill -HUP ${SAPDPID}
			[ ! -z "${SAPDPID}" ] && kill -14 ${SAPDPID}
			return 0
		  fi
		fi
		;;
	*)
		$PFMT -g $MSG_ENARG $1
		exit 1;
		;;
	esac
}

#
#  Disable TCP interface
#
disable()
{
	case "$1" in
	tcp)
		grep "^${SERVICE}" /etc/inet/inetd.conf > /dev/null
		[ $? -eq 0 ] && {
			INETTMP=/var/tmp/${SERVICE}.$$
			grep -v "^${SERVICE}" /etc/inet/inetd.conf >${INETTMP}
			cat ${INETTMP} >/etc/inet/inetd.conf
			rm -f ${INETTMP}
		}
		#
		#  Give the inetd a kick to use new configuration
		#
		INETPID=`ps -luroot | grep " inetd$" | awk '{print $4}'`
		[ ! -z "${INETPID}" ] && kill -HUP ${INETPID}
		return 0
		;;
	spx)
		#
		#  Remove entries from /etc/nwnetd.conf, if there
		#
		grep "^${SERVICE}" /etc/nwnetd.conf > /dev/null 2>&1
		[ $? -eq 0 ] && {
			INETTMP=/var/tmp/${SERVICE}.$$
			grep -v "^${SERVICE}" /etc/nwnetd.conf >${INETTMP}
			cat ${INETTMP} >/etc/nwnetd.conf
			rm -f ${INETTMP}
		}
		#
		#  Give the sapd a kick to use new configuration
		#
		if [ -f /usr/sbin/nwsaputil ]
		then
			/usr/sbin/nwsaputil -d -t ${SPX_SERVICE_HEX} -n ${MY_HOSTNAME} -s ${SPX_PORT_HEX}
			return $?
		else if [ -f /var/spool/sap/out/${UW_SERVICE_HEX} ]
		  then
			rm -f /var/spool/sap/out/${SPX_SERVICE_HEX}
			SAPDPID=`ps -luroot | grep " sapd$" | awk '{print $4}'`
			[ ! -z "${SAPDPID}" ] && kill -HUP ${SAPDPID}
			[ ! -z "${SAPDPID}" ] && kill -14 ${SAPDPID}
		  fi
		fi
		return 0
		;;
	*)
		$PFMT -g $MSG_DISARG $1
		exit 1;
		;;
	esac
}

#
#  Determine if a networking interface exists
#	Accepts either "tcp" or "spx"
#	Returns 1 if it IS there
#	Returns 0 if it IS NOT there
#
isthere()
{
	case "$1" in
	tcp)
		[ -f /etc/inet/inetd.conf ] && return 1
		return 0
		;;
	spx)
		[ -f /etc/nwnetd.conf ] && return 1
		return 0
		;;
	*)
		$PFMT -g $MSG_ISARG $1
		exit 1;
		;;
	esac
}

#
#  Query all aspects of this system
#  	Returns number of enabled protocols
#
query()
{
	ENABLED=0
	#
	#  1. Enabled Networks
	#  2. Spooled Packages per locale
	#
	isthere spx
	[ $? ] && {
		$PFMT -s INFO -g $MSG_ENABLE "SPX"
		grep "^${SERVICE}" /etc/nwnetd.conf > /dev/null 2>&1
		[ $? = 0 ] && {
			$PFMT -s INFO -g $MSG_SENABLED ${SERVICE} "SPX"
			ENABLED=`expr ${ENABLED} + 1`
		}
	}
	isthere tcp
	[ $? ] && {
		$PFMT -s INFO -g $MSG_ENABLE "TCP"
		grep "^${SERVICE}" /etc/inet/inetd.conf > /dev/null 2>&1
		[ $? = 0 ] && {
			$PFMT -s INFO -g $MSG_SENABLED ${SERVICE} "TCP"
			ENABLED=`expr ${ENABLED} + 1`
		}
	}

	curr_dir=`pwd`
	cd ${SPOOL_BASE}
	for A_LANG in */*
	do
	    [ -d "$A_LANG" ] || continue
	    [ -r "${A_LANG}/as.package" ] && {
		    echo "`$GETTXT $TXT_LOCALE` `basename ${A_LANG}`\t`$GETTXT $TXT_AS`"
	    }
	    [ -r "${A_LANG}/pe.package" ] && {
		    echo "`$GETTXT $TXT_LOCALE` `basename ${A_LANG}`\t`$GETTXT $TXT_PE`"
	    }
	done
	cd $curr_dir

	return ${ENABLED}
}

#
#  Print Usage Statement
#
usage()
{
	$PFMT -s action -g $MSG_U1
	$PFMT -s action -g $MSG_U2
	$PFMT -s action -g $MSG_U3
	$PFMT -s action -g $MSG_U4
	$PFMT -s action -g $MSG_U5

	exit 1
}

#
#  main()
#
#  Parse command line
#
while getopts 'euqL:p:n:l:d:c:s:?' c
do
	case "${c}" in
	e)		# Enable
		ENABLE=YES
		;;
	u)		# Disable
		DISABLE=YES
		;;
	q)		# Query
		QUERY=YES
		;;
	L)		# Locale
		LOCALE=${OPTARG}
		;;
	p)		# Product Name
		PRODUCT="${OPTARG}"
		;;
	n)		# Network Type
		CHECK=${OPTARG}
		[ "${CHECK}" != "tcp" -a "${CHECK}" != "spx" ] && usage
		NETWORK="${NETWORK} ${CHECK}"
		;;
	l)		# What to Load (sa, pe)
		CHECK=${OPTARG}
		[ "${CHECK}" != "as" -a "${CHECK}" != "pe" ] && usage
		LOAD="${LOAD} ${CHECK}"
		;;
	d)		# Device
		DEVICE=${OPTARG}
		DEVTYPE=`devattr ${DEVICE} type`
		[ $? != 0 ] && {
			$PFMT -g $MSG_NODEV ${DEVICE}
			exit 19		# ENODEV
		}
		;;
	c)		# Copy (spool or mount)
		CHECK=${OPTARG}
		[ "${CHECK}" != "spool" -a "${CHECK}" != "mount" ] && usage
		COPY=${CHECK}
		;;
	s)		# Server
		SERVER=${OPTARG}
		;;
	\?)		# Help
		usage
		;;
	*)		# Bad option
		usage
		;;
	esac
done

#
#  Now to test for all invalid combinations
#
ALLOPTS="${QUERY}${ENABLE}${DISABLE}${LOCALE}${PRODUCT}${NETWORK}${LOAD}${DEVICE}${COPY}${SERVER}"
[ -z "${ALLOPTS}" ] && {
	$PFMT -g $MSG_NOOPT
	usage
}

OTHERS="${ENABLE}${DISABLE}${LOCALE}${PRODUCT}${NETWORK}${LOAD}${DEVICE}${COPY}${SERVER}"
[ ! -z "${QUERY}" -a ! -z "${OTHERS}" ] && {
	$PFMT -g $MSG_BADQ
	usage
}

[ ! -z "${ENABLE}" -a ! -z "${DISABLE}" ] && {
	$PFMT -g $MSG_ENUN
	usage
}
OTHERS="${ENABLE}${DISABLE}"
[ ! -z "${NETWORK}" -a -z "${OTHERS}" ] && {
	$PFMT -g $MSG_BADN
	usage
}
[ ! -z "${DEVICE}" -a ! -z "${SERVER}" ] && {
	$PFMT -g $MSG_DS
	usage
}
[ ! -z "${DEVICE}" -a -z "${COPY}" ] && {
	$PFMT -g $MSG_CD
	usage
}
[ "${COPY}" = "mount" ] && {
	[ ! -z "${SERVER}" ] && {
		$PFMT -g $MSG_NMOUNT
		usage
	}
	[ "${DEVTYPE}" != "cdrom" ] && {
		$PFMT -g $MSG_NOTCD
		usage
	}
	BDEV=`devattr ${DEVICE} bdevice`
}
[ -z "${LOCALE}" ] && {
	LOCALE=${LANG}
}
[ ! -z "${COPY}" -a -z "${LOCALE}" ] && {
	$PFMT -g $MSG_NLOCALE
	usage
}

[ -z "${PRODUCT}" ] && PRODUCT="${DFLT_PRODUCT}"

[ ! -z "${COPY}" -a -z "${PRODUCT}" ] && {
	$PFMT -g $MSG_NPROD
	usage
}
[ -z "${COPY}" -a ! -z "${SERVER}" ] && {
	COPY="spool"
}


#
#  Now do the actions.
#
[ ! -z "${QUERY}" ] && {
	query
	exit $?
}

[ ! -z "${ENABLE}" ] && {
	[ -z "${NETWORK}" ] && NETWORK="tcp spx"
	for PROTOCOL in ${NETWORK}
	do
		enable ${PROTOCOL}
	done
	exit $?
}
[ ! -z "${DISABLE}" ] && {
	[ -z "${NETWORK}" ] && NETWORK="tcp spx"
	for PROTOCOL in ${NETWORK}
	do
		disable ${PROTOCOL}
	done
	exit $?
}

#
#  Do any loading of spftware we were requested to do
#
[ ! -z "${LOAD}" ] && {
	case "${COPY}" in
	mount)
		mntpkg
		;;
	spool)
		if [ ! -z "${SERVER}" ]
		then
			copypkg
		else
			spoolpkg
		fi
		;;
	*)
		$PFMT -g $MSG_NCOPY ${COPY}
		exit 100	# ?
		;;
	esac
}
