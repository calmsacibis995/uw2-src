#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/hba/adss/adss.pkg/postinstall.sh	1.1"

do_install () {

	${CONFBIN}/idinstall -P ${PKGINST} -a ${1} > ${ERR} 2>&1
	RET=$?
	if [ ${RET} != 0 ]
	then
		${CONFBIN}/idinstall -P ${PKGINST} -u ${1} > ${ERR} 2>&1
		RET=$?
	fi

	if [ ${RET} != 0 ]
	then
		message "The installation cannot be completed due to \
an error in the driver installation during the installation of the ${1} \
module of the ${NAME}.  The file ${ERR} contains the errors."
			exit ${FAILURE}
	fi
	mv disk.cfg /etc/conf/pack.d/${1}
	mv ${1}.h /usr/include/sys
}

FAILURE=1	# fatal error

CONFDIR=/etc/conf
CONFBIN=${CONFDIR}/bin
ERR=/tmp/err.out

for MODULE in adss
do
	cd /tmp/${MODULE}
	do_install ${MODULE}
done

if test -f /bin/grep
then
	if grep adss /etc/loadmods >/dev/null
	then
		message -d "It appears the adss driver has either been previously \
installed or another driver is using the same name."
		message -d "Installation will continue"
	else
		cat /tmp/adss/loadmods >>/etc/loadmods
	fi
else
	cat /tmp/adss/loadmods >>/etc/loadmods
fi

installf -f $PKGINST 

removef ${PKGINST} /tmp/adss >/dev/null 2>&1
removef -f ${PKGINST} >/dev/null 2>&1
