#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)initpkg:i386/cmd/initpkg/rc2.d/:mk.rc2.d.sh	1.12.14.10"

if i386
then
	STOPLST="30fumounts 40rumounts"
	STARTLST="01MOUNTFSYS 02mse 05RMTMPFILES \
	15MLD 15mkdtab 20sysetup"
else
	STOPLST="30fumounts 40rumounts"
	STARTLST=" 00firstcheck 01MOUNTFSYS 05RMTMPFILES 08ports 10disks \
	15MLD 15mkdtab 20sysetup 23ttymap"
fi


INSDIR=${ROOT}/${MACH}/etc/rc2.d
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
	for f in ${STARTLST}
	do 
		name=`echo $f | sed -e 's/^..//'`
		rm -f ${INSDIR}/S$f
		ln ${ROOT}/${MACH}/etc/init.d/${name} ${INSDIR}/S$f
	done
fi
