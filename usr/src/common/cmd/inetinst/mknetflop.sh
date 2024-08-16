#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)inetinst:mknetflop.sh	1.2"
#!/usr/bin/xksh

CWD=`pwd`
TMPDIR=/tmp/mknetflop.$$
trap "cd ${CWD}; umount /install; rm -rf ${TMPDIR}; exit" 0 1 2 3 15

#
#  mknetflop
#
#  Used to cut a new Network Installation Utilities floppy from
#    1) an existing Network Installation Utilities floppy
#    2) a new directory containing idinstallable stuff -or-
#	a DOS IHV Networking Floppy
#
#  See also:
#      config(4)
#

#
#  Usage
#
function Usage
{
	echo "Usage: $0 -s source_device -t target_device"
	echo " where "
	echo "        source_device is the device name/alias to find"
	echo "        the new driver and config files"
	echo 
	echo "        target_device is the device on which to read the"
	echo "        old network install floppy and write the new one."
	exit 22 # EINVAL
}

#
#  Function IsFloppy
#
#  Determines whether device alias passed as $1 is a floppy drive
#  RETURNS: 0 if device is a floppy
#           1 if device is NOT a floppy
#  SIDE EFFECTS:	Sets the following variables
#
#			Capacity	Capacity of drive (in blocks)
#			BDevName	Full pathname of block device
#			CDevName	Full pathname of block device
#			FmtCmd		Format Command for device
#
function IsFloppy
{
	DeviceAlias=${1}
	Capacity=`/bin/devattr ${DeviceAlias} capacity`
	[ $? != 0 ] && {
		return 1
	}
	BDevName=`/bin/devattr ${DeviceAlias} bdevice`
	CDevName=`/bin/devattr ${DeviceAlias} cdevice`
	FmtCmd=`/bin/devattr ${DeviceAlias} fmtcmd`
	return 0
}


#
#  Create a directory, with error checking
#
function MNmkdir
{
	NEWDIR=${1}

	#
	#  Check for directory structure; if it's not there, make it.
	#
	if [ ! -d ${NEWDIR} ]
	then
		/bin/mkdir -p ${NEWDIR}
		[ $? != 0 ] && {
			echo "Couldnt mkdir ${NEWDIR}"
			exit 13	# EACCES
		}
	else
		echo "Directory ${NEWDIR} already exists"
		exit 17 # EEXIST
	fi
	return 0
}

#
#  This function lets user select one or more packages from a list
#
function SelectPkgs
{
	echo "The following packages are available:"
	Choice=0
	while read PackageName
	do
		let Choice+=1
		PackageList[${Choice}]=`basename ${PackageName}`
		echo "${Choice}\t${PackageList[${Choice}]}"
	done < ${1}
	echo

	Chosen=0
	while [ ${Chosen} = 0 ]
	do
		echo "Select package(s) you wish to process: \c"
		read PkgList

		[ ! -z "${PkgList}" ] && {
			for PkgNum in ${PkgList}
			do
				Chosen=1
				[ ${PkgNum} -lt 1 -o ${PkgNum} -gt ${Choice} ] && {
					echo "Bad numeric choice specification" >&2
					Chosen=0
				}
			done
		}
	done
}

#
#  Make sure the syntax is correct:
#	mknetflop -d disk -c config_dir -r root_dir
#
#  main()
#
#  Parse command line
#

Mflag=0
[ "$1" = "" ] && Usage 
while getopts 'ms:t:?' c
do
	case "${c}" in
	m)		# modify only
		Mflag=1
		;;
	s)		# diskette1 or diskette2
		SourceDevice=${OPTARG}
		[ -z "${SourceDevice}" ] && Usage
		;;
	t)		# config file directory
		TargetDevice=${OPTARG}
		[ -z "${TargetDevice}" ] && Usage
		;;
	\?)		# Help
		Usage
		;;
	*)		# Bad option
		Usage
		;;
	esac
done

#
#  Check to see that TargetDevice is a hi-density floppy device
#
IsFloppy "${TargetDevice}"
if [ $? = 0 ]
then
	[ "${Capacity}" -lt "2800" ] && {
		echo "${TargetDevice} is not a high-density floppy device." >&2
		exit 19 # ENODEV
	}
else
	echo "${TargetDevice} is not a high-density floppy device." >&2
	exit 19 # ENODEV
fi

#
#  Check to see that the source_device is a directory with driver stuff
#  or a floppy
#
SourceIsFloppy=0
[ -d "${SourceDevice}" ] && {
	for IdFile in Driver.o Space.c Node System Master
	do
		[ ! -f ${SourceDevice}/${IdFile} ] && {
			echo "File ${IdFile} not found in ${SourceDevice}" >&2
			exit 2 # ENOENT
		}
	done

	#
	#  Make sure that location/count info will be consistent if
	#  SourceDevice is a directory or a floppy.
	#
	PkgList=1
	PackageList[1]=`/bin/basename ${SourceDevice}`
	DriverName[1]=`/bin/basename ${SourceDevice}`
	PkgPrefix=${SourceDevice}

}

IsFloppy ${SourceDevice}
[ $? = 0 ] && SourceIsFloppy=1

#
#  Create temporary working space
#
MNmkdir ${TMPDIR}

#
#  If the SourceDevice is a floppy, make sure that it's a dosfs filesystem
#  with at least one *.pkg file on it.  If so, select which file(s) to use,
#  and pkgtrans them in to the TMPDIR.
#
[ ${SourceIsFloppy} != 0 ] && {

	#
	#  Make sure it's a dosfs.
	#
	Mounted=0
	while [ ${Mounted} = 0 ]
	do
	echo "Make sure the DOS Networking IHV Diskette is in ${SourceDevice}"
	echo "and press <Enter> to continue. \c"
	read X

	IsFloppy ${SourceDevice}
	/sbin/mount -r -F dosfs ${BDevName} /install > /dev/null 2>&1
	if [ $? != 0 ]
	then
		echo "Could not mount ${SourceDevice} as a DOS filesystem." >&2
		echo >&2
	else
		/bin/ls -1 /install/*.pkg > ${TMPDIR}/pkgs 2>/dev/null
		[ $? != 0 ] && {
		echo "DOS diskette does not contain a UnixWare driver." >&2
		echo >&2
		exit 2 # ENOENT
		}
		Mounted=1
	fi
	done

	#
	#  Choose which package(s) to use for new net install floppy
	#
	SelectPkgs ${TMPDIR}/pkgs

	#
	#  pkgtrans the selected packages to the TMPDIR
	#
	MNmkdir ${TMPDIR}/pkg

	for PkgNum in ${PkgList}
	do
		echo "Transferring ${PackageList[${PkgNum}]}..."
		echo "all" | /bin/pkgtrans /install/${PackageList[${PkgNum}]} \
			${TMPDIR}/pkg >/dev/null 2>&1
		/bin/ls -1t ${TMPDIR}/pkg | read DriverName[${PkgNum}]
	done

	#
	#  Umount the dosfs filesystem.
	#
	/sbin/umount /install > /dev/null 2>&1
}

[ "${Mflag}" = 0 ] && {
#
#  Read in a DD image of the original net install floppy
#
echo "Make sure the original Network Installation Utilities diskette is"
echo "in ${TargetDevice} and press <Enter> to continue... \c"
read X

IsFloppy ${TargetDevice}
echo "Copying in original diskette image.  Please wait..."
/bin/dd if=${CDevName} of=${TMPDIR}/netflop.ORIG bs=36b
echo "Copy is complete."
echo
}

#
#  Format and write the DD image onto a new net install floppy.
#
echo "Make sure the new Network Installation Utilities diskette is"
echo "in ${TargetDevice} and press <Enter> to continue... \c"
read X

[ "${Mflag}" = 0 ] && {
IsFloppy ${TargetDevice}
eval "${FmtCmd}"

echo "Copying original diskette image to new diskette.  Please wait..."
/bin/dd of=${CDevName} if=${TMPDIR}/netflop.ORIG bs=36b
echo "Copy is complete."
echo
}

#
#  Now mount the new network install floppy.
#
IsFloppy ${TargetDevice}
/sbin/mount -F s5 ${BDevName} /install > /dev/null 2>&1
[ $? != 0 ] && {
	echo "Could not mount new Network Installation Utilities Diskette.">&2
	exit 89 # ENOSYS
}

#
#  Copy in the xtra.z utilities
#
cd ${TMPDIR}
/usr/bin/cpio -icd -DZ < /install/xtra.z > /dev/null 2>&1
[ $? != 0 ] && {
	echo "Could not read new Network Installation Utilities Diskette.">&2
	exit 2 # ENOENT
}

#
#  Copy in the config.z config files
#
MNmkdir ${TMPDIR}/config
cd ${TMPDIR}/config
/usr/bin/cpio -icd -DZ < /install/nics/config.z > /dev/null 2>&1
[ $? != 0 ] && {
	echo "Could not read new Network Installation Utilities Diskette.">&2
	exit 2 # ENOENT
}

#
#  For each driver selected, idinstall, configure, idbuild, cpioout
#  the driver, and then put the config file in it's staging area.
#
for PkgNum in ${PkgList}
do
	#
	#  If we copied a pkg in from the DOS IHV floppy, specify the
	#  source directory.
	#
	[ ${SourceIsFloppy} != 0 ] &&
		PkgPrefix="${TMPDIR}/pkg/${DriverName[${PkgNum}]}/root/tmp/nics/${DriverName[${PkgNum}]}"

	#
	#  idinstall the new driver
	#
	cd ${PkgPrefix}

	#
	#  Make sure driver is turned on in System file.
	#
	mv System System.$$

	cat System.$$ | \
	/usr/bin/awk 'BEGIN { OFS="\t";}
	NF==11   {
		$2="Y";
		print $0
		next
	}
	{print $0}' > System
	rm System.$$

	#
	#  Zero out major number so we can idinstall successfully
	#
	mv Master Master.$$

	cat Master.$$ | \
	/usr/bin/awk 'BEGIN { OFS="\t";}
	NF==6   {
		$6="0";
		print $0
		next
	}
	{print $0}' > Master
	rm Master.$$

	/etc/conf/bin/idinstall -a -k ${DriverName[${PkgNum}]} || /etc/conf/bin/idinstall -u -k ${DriverName[${PkgNum}]}

	#
	#  Configure new driver to match the net install major numbers
	#
	/bin/mv /etc/conf/mdevice.d/${DriverName[${PkgNum}]} \
		/etc/conf/mdevice.d/${DriverName[${PkgNum}]}.$$

	/bin/cat /etc/conf/mdevice.d/${DriverName[${PkgNum}]}.$$ | \
	/usr/bin/awk 'BEGIN { OFS="\t";}
	NF==6   {
		$3=$3"k"
		$6="72-76";
		print $0
		next
	}
	{print $0}' > /etc/conf/mdevice.d/${DriverName[${PkgNum}]}
	/bin/rm /etc/conf/mdevice.d/${DriverName[${PkgNum}]}.$$

	#
	#  idbuild the new driver.
	#
	/etc/conf/bin/idbuild -c -M ${DriverName[${PkgNum}]}

	#
	#  Now put all of the parts into a directory subtree and zip them
	#
	/bin/mkdir -p ${TMPDIR}/${DriverName[${PkgNum}]}/etc/conf/mod.d
	/bin/cp /etc/conf/mod.d/${DriverName[${PkgNum}]} ${TMPDIR}/${DriverName[${PkgNum}]}/etc/conf/mod.d/
	/bin/mkdir -p ${TMPDIR}/${DriverName[${PkgNum}]}/etc/conf/drvmap.d
	/bin/cp /etc/conf/drvmap.d/${DriverName[${PkgNum}]} ${TMPDIR}/${DriverName[${PkgNum}]}/etc/conf/drvmap.d/
	/bin/mkdir -p ${TMPDIR}/${DriverName[${PkgNum}]}/etc/conf/sdevice.d
	/bin/cp /etc/conf/sdevice.d/${DriverName[${PkgNum}]} ${TMPDIR}/${DriverName[${PkgNum}]}/etc/conf/sdevice.d/

	#
	#  Any extra files in the package need to be packed up too
	#
	. ${DriverName[${PkgNum}]}.bcfg
	echo ${EXTRA_FILES} | /bin/cpio -pdum ${TMPDIR}/${DriverName[${PkgNum}]}/ > /dev/null 2<&1
	
	PATH=${PATH}:${TMPDIR}
	export PATH
	cd ${TMPDIR}/${DriverName[${PkgNum}]}
	/bin/find . -print | /usr/bin/sh ${TMPDIR}/cpioout > \
		${TMPDIR}/${DriverName[${PkgNum}]}.z
	/bin/cp ${TMPDIR}/${DriverName[${PkgNum}]}.z /install/nics/
	
	#
	#  Put the config files in the config directory
	#
	if [ ${SourceIsFloppy} != 0 ]
	then
		ConfigSrc="${TMPDIR}/pkg/${DriverName[${PkgNum}]}/root/tmp/nics/${DriverName[${PkgNum}]}/*.bcfg"
	else
		ConfigSrc="${SourceDevice}/*.bcfg"
	fi

	for ConfigFile in ${ConfigSrc}
	do
		ConfigName=`echo ${ConfigFile} | /bin/basename | /bin/sed -e "s/\.bcfg//"`
		/bin/cp ${ConfigFile} ${TMPDIR}/config/${ConfigName}
	done

	#
	#  Copy the new driver.z to the new net install floppy
	#
	/bin/cp ${TMPDIR}/pkg/${DriverName[${PkgNum}]}/${DriverName[${PkgNum}]}.z /install/nics/

	#
	#  pkginstall -u the new driver so that we don't foul up the system
	#
	/etc/conf/bin/idinstall -d ${DriverName[${PkgNum}]}
done

#
#  zip up all the config files and
#  copy the new config.z to the new net install floppy
#
cd ${TMPDIR}
/usr/bin/find config -print | /usr/bin/sh ${TMPDIR}/cpioout > \
	${TMPDIR}/config.z
/bin/cp ${TMPDIR}/config.z /install/nics/

#
#  umount the net install floppy - we're done!
#
/sbin/umount /install

echo "Configuration completed.  Your new Network Installation Utilities"
echo "diskette is ready for use."
