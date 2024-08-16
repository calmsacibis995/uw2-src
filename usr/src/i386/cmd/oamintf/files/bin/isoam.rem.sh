#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:i386/cmd/oamintf/files/bin/isoam.rem.sh	1.1.3.2"
#ident	"$Header: $"

# Determine if a package is a removable OAM style package; RC=0 means yes

#
# Test for existence of 4.0 /var/sadm/pkg/<pkgname>
#

OAMSTYLE=""
OTHSTYLE=""
TESTDEV=`echo $2 | cut -f2 -d" "`

echo ${2} | fgrep "spool" > /dev/null
SPOOLED=$?
 
for NAME in `echo "${1}" | tr "," " "`
do
        if [ $NAME = "all" ]
        then
                OAMSTYLE=" "
        else
                if [ $SPOOLED -ne 0 -a -d /var/sadm/pkg/${NAME} ]
                          || [ $SPOOLED -eq 0 -a -d /var/spool/pkg/${NAME} ]
                then
                        OAMSTYLE="${OAMSTYLE} ${NAME}"
                else
                        OTHSTYLE="${OTHSTYLE} ${NAME}"
                fi
done

if [ ! -z "${OAMSTYLE}" ]
then
	pkgrm ${2} ${OAMSTYLE}
fi

if [ ! -z "${OTHSTYLE}" ]
then
	for PKG in ${OTHSTYLE}
	do
		removepkg ${PKG}
	done
fi

exit 0
