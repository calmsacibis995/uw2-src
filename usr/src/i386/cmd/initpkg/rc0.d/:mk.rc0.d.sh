#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)initpkg:i386/cmd/initpkg/rc0.d/:mk.rc0.d.sh	1.7.10.6"
#ident "$Header: :mk.rc0.d.sh 1.2 91/04/26 $"

STARTLST= 
STOPLST="00ANNOUNCE 02mse 50fumounts 60rumounts 70cron"

INSDIR=${ROOT}/${MACH}/etc/rc0.d
if u3b2 || i386
then
	if [ ! -d ${INSDIR} ] 
	then 
		mkdir ${INSDIR} 
		eval ${CH}chmod 755 ${INSDIR}
		eval ${CH}chgrp sys ${INSDIR}
		eval ${CH}chown root ${INSDIR}
	fi 
	for f in ${STOPLST}
	do 
		name=`echo $f | sed -e 's/^..//'`
		rm -f ${INSDIR}/K$f
		ln ${ROOT}/${MACH}/etc/init.d/${name} ${INSDIR}/K$f
	done
fi
