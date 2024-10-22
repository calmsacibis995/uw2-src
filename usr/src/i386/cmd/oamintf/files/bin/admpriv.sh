#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:i386/cmd/oamintf/files/bin/admpriv.sh	1.1.3.1"
#ident	"$Header: $"

ADMF=/etc/.useradm
LCKF=/etc/adm.lock
PSWF=/etc/passwd
TMP=/tmp/.useradm
LOOP=20

if [ $# -eq 2 ]
then
	case "$2" in
	Yes)
		if [ ! -z "`grep $1 ${PSWF}`" -a -z "`grep $1 ${ADMF} 2>/dev/null`" ]
		then
			echo "$1 base-adm" 2>/dev/null >> ${ADMF}
		fi
		break
		;;
	No)
		if [ ! -z "`egrep \"^$1 \" ${ADMF} 2>/dev/null`" ]
		then
			while [ -f "${LCKF}" -a "${LOOP}" -ne 0 ]
			do
				sleep 1
				LOOP=`expr ${LOOP} - 1`
			done

			if [ "${LOOP}" -ne 0 -a ! -f "${LCKF}" ]
			then
				touch ${LCKF} > /dev/null 2>&1
				if [ $? -eq 0 ]
				then
					umask 022
					grep -v "$1 base-adm" ${ADMF} > ${TMP}
					mv ${TMP} ${ADMF}
					rm -f ${LCKF}
				else
					exit 1
				fi
			fi
		fi
		break
		;;
	V)
		if [ ! -z "`grep $1 ${PSWF}`" -a ! -z "`grep $1 ${ADMF}`" ]
		then
			echo "Yes"
		else
			echo "No"
		fi
		break
		;;
	*)
		break
		;;
	esac
fi
