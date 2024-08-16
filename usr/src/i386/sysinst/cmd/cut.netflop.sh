#!/usr/bin/ksh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:cmd/cut.netflop.sh	1.33"

#
#  cut.netflop.sh:
#  Cut a Network Installation Utilities floppy -
#	- Create staging directory ($PROTO/net.flop) structure
#	- Hand create/modify sone network config files
#	- Build loadable modules for networking (tcp, spx)
#	- Build loadable modules for NICs
#	- Create zipped cpio archives for:
#		- common code
#		- tcp-specific bits
#		- spx-specific bits
#		- each NIC driver & its attendant files
#		- config files from NICs pkg
#	- Cut the floppy
#

NETFLOP_ROOT=${PROTO}/net.flop		# Staging directory
FLOPPROTO=/tmp/flopproto$$		# Prototype file for s5 filesystem
trap "rm -f $FLOPPROTO; rm -f ${FLOPPROTO}.tmp; exit" 0 1 2 3 15

. ${ROOT}/${MACH}/var/sadm/dist/rel_fullname
[ -z "${REL_FULLNAME}" ] && {
	echo "REL_FULLNAME not set.  Please set and reinvoke this command."
	exit 1
}

#
#  Right up front, we need to create our prototype file for cutting
#  the Network Installation Utilities floppy; we write to this file
#  later before we use it.
#
cp $PROTO/desktop/files/netflop.proto ${FLOPPROTO}.tmp

#
#  Function to prompt a user what diskette drive they're going to use
#  to cut this Network Install Floppy (lifted from cut.flop)
#
ask_drive()
{
	#
	#  We need a high-capacity drive to fit the net install floppy
	#  
	DONE=0
	while [ "${DONE}" = "0" ]
	do
	[ "$MEDIUM" = "" ] && {
		echo "Please enter diskette1 or diskette2 (default is diskette1): \c"
		read MEDIUM
		[ "$MEDIUM" = "" ] && MEDIUM="diskette1"
	}
	FDRIVE_TMP=`devattr $MEDIUM fmtcmd|cut -f 3 -d " "|sed 's/t//'`
	FDRIVE=`basename $FDRIVE_TMP`
	BLKS=`devattr $MEDIUM capacity`
	if [ "${BLKS}" -le "2800" ]
	then
		echo "You must choose a high-capacity drive.  The drive"
		echo "you selected will only hold ${BLKS} blocks."
	else
		DONE=1
	fi
	done

	BLKCYLS=`devattr $MEDIUM mkfscmd|cut -f 7 -d " "`
	echo "${FDRIVE}\t${BLKCYLS}\t${BLKS}" >&2
}


#
#  Update mod_register file with each module's info
#
mod_reg_updt()
{
	DRIVER=$1
	DIR=$2
	MDEV="${DIR}/../mdevice.d"
	MOD_OUT="${NETFLOP_ROOT}/etc/conf/mod_register"

	#
	#  Yank the major dev range out of the Master file
	#
	MAJORRANGE=`tail -1 ${MDEV}/${DRIVER} | cut -f 6`
	if [ "${MAJORRANGE}" = "0" ] 
	then
		#
		#  Streams modules (type 3) are not 'real' devices
		#
		echo "3:1:${DRIVER}:${DRIVER}" >> ${MOD_OUT}
	else
		#
		#  Real devices are type 5
		#
		echo "${MAJORRANGE}" | sed "s/-/ /" > /tmp/rm.$$
		read START END < /tmp/rm.$$ 
		rm /tmp/rm.$$
		[ -z "${END}" ] && END=${START}
		while [ ${START} -le ${END} ]
		do
			echo "5:1:${DRIVER}:${START}" >> ${MOD_OUT}
			START=`expr ${START} + 1`
		done
	fi
}

#
#  This function allows Networking drivers to be turned off and on so that
#  loadable modules may be built without collision.
#  	Usage:
#		switch_driver driver_name turnonnic      or
#		switch_driver driver_name turnon         or
#		switch_driver driver_name turnoff
#  (turnonnic does some extra work to munge a NIC driver, like making sure
#   that it's at unused IO/RAM/ Addresses & IRQ, so driver can be idbuilt)
#
switch_driver()
{
	DRIVER=$1
	ACTION=$2
	SW_TMP="/tmp/${DRIVER}.sys.$$"
	MDEV_ORIG="${ROOT}/${MACH}/etc/conf/mdevice.d"
	SDEV_ORIG="${ROOT}/${MACH}/etc/conf/sdevice.d"
	PACK_ORIG="${ROOT}/${MACH}/etc/conf/pack.d"
	NODE_ORIG="${ROOT}/${MACH}/etc/conf/node.d"
	NODE="${ROOT}/.${MACH}/etc/conf/node.d"
	PACK="${ROOT}/.${MACH}/etc/conf/pack.d"
	SDEV="${ROOT}/.${MACH}/etc/conf/sdevice.d"
	MDEV="${ROOT}/.${MACH}/etc/conf/mdevice.d"
	MOD="${ROOT}/.${MACH}/etc/conf/mod.d"

	#
	#  The real sdevice file from the tree is in SDEV_ORIG, so no matter
	#  what we're doing here, we copy that into our working directory.
	#  (Same for the mdevice file!)
	#
	cp ${SDEV_ORIG}/${DRIVER} ${SDEV}/${DRIVER}
	if [ $? != 0 ]
	then
		cp ${SDEV_ORIG}/.${DRIVER} ${SDEV}/${DRIVER}
		if [ $? != 0 ]
		then
			echo "Could not find ${SDEV_ORIG}/${DRIVER}"
			exit 1
		fi
	fi

	cp ${MDEV_ORIG}/${DRIVER} ${MDEV}/${DRIVER}
	if [ $? != 0 ]
	then
		cp ${MDEV_ORIG}/.${DRIVER} ${MDEV}/${DRIVER}
		if [ $? != 0 ]
		then
			echo "Could not find ${MDEV_ORIG}/${DRIVER}"
			exit 1
		fi
	fi

	#
	#  Make mods to the Master file - make sure that the Major
	#  numbers are 72-76.
	#
	awk 'BEGIN	{
				condition="'$ACTION'";
				OFS="\t";
			}
	NF==6		{ 
			if (condition == "turnonnic" )
			{
				$6="72-76";
			}	
			print $0 
			next
			}
			{ print $0 }' \
			${MDEV}/${DRIVER} > ${SW_TMP} && \
				mv ${SW_TMP} ${MDEV}/${DRIVER}
	#
	#  Make mods to the System file - turn on (Y) or off (N),
	#  if it's an NIC driver, also make sure that we give it
	#  nonconflicting values for RAM, IO, IRQ
	#
	awk 'BEGIN	{
				condition="'$ACTION'";
				OFS="\t";
			}
	NF==11		{ 
			if (condition == "turnonnic" )
			{
				$2="Y";
				#
				#  If itype=0, then this card should not have
				#  IRQ/Addresses set, or else idbuild fails.
				#
				if ( $5 != "0" )
				{
					$6="9";
					$7="280";
					$8="29f";
					$9="D0000";
					$10="D1fff";
				}
				$11="-1";
				condition = "turnoff";		
			}	
			else
				if (condition == "turnon" )
				{
				$2="Y";
				}
				else
				{
					$2="N";
				}
			print $0 
			next
			}
			{ print $0 }' \
			${SDEV}/${DRIVER} > ${SW_TMP} && \
				mv ${SW_TMP} ${SDEV}/${DRIVER}

	#
	#  We need to play a bit with the networking driver space.c here
	#  so that odm can modify it successfully.  We redefine a static
	#  char string to be a usable external.
	#
	if [ "${ACTION}" = "turnonnic" ]
	then
		[ ! -d ${PACK}/${DRIVER} ] && {
			mkdir -p ${PACK}/${DRIVER} || {
				echo "Couldnt mkdir ${PACK}/${DRIVER}"
				exit 1
			}
			cp ${PACK_ORIG}/${DRIVER}/Driver.o ${PACK}/${DRIVER}
		}
		cp ${NODE_ORIG}/${DRIVER} ${NODE}/${DRIVER}
		cp ${PACK_ORIG}/${DRIVER}/space.c ${PACK}/${DRIVER}/space.c
	fi
}

# main()

#
#  Variables for our convenience
#
PATH=$PROTO/bin:$PROTO/cmd:$PATH export PATH
TCP_DRIVERS="app arp icmp inet ip route sockmod tcp timod tirdwr udp"
SPX_DRIVERS="uni ipx nspx ripx"
NET_DRIVERS="net lsl msm ethtsm toktsm odisr"

#
#  To determine the list of NIC drivers we need, look in the config
#  directory of the NICS package.
#
nl='
'
IITMP=/tmp/inetinst.$$
mkdir -p ${IITMP}
> ${IITMP}/nic_list
OIFS=${IFS}
IFS="${IFS}${nl}"

#
#  Go through the NICS pkg config files, creating a list of what
#  files to put into cpio archives for each DRIVER_NAME, and a list of
#  DRIVER_NAMEs.
#

for NICS in ${ROOT}/${SPOOL}/nics/install/config/*
do
	unset DRIVER_NAME EXTRA_FILES
	. ${NICS}

	echo ${DRIVER_NAME} >> ${IITMP}/nic_list
	echo "etc/conf/mod.d/${DRIVER_NAME}" >> ${IITMP}/${DRIVER_NAME}.files
	echo "etc/conf/drvmap.d/${DRIVER_NAME}" >> ${IITMP}/${DRIVER_NAME}.files
	for FILE in ${EXTRA_FILES}
	do
		echo $FILE | sed -e "s,^/,," >> ${IITMP}/${DRIVER_NAME}.files
	done
done
IFS=${OIFS}

for FILE in ${IITMP}/*
do
	sort -u -o ${FILE} ${FILE}
done

NIC_DRIVERS=`cat ${IITMP}/nic_list`


#
#  Make sure tht appropriate environment variables are set
#
[ -z "${PROTO}" ] && {
        echo "PROTO is not set."
        exit 1
}

#
#  Make sure that our staging directory exists
#
[ ! -d "${NETFLOP_ROOT}" ] && {
        mkdir -p ${NETFLOP_ROOT}
        RET=$?
        [ ${RET} != 0 ] && {
                echo "Couldn't create directory ${NETFLOP_ROOT}."
                exit 1
        }
}

#
#  Create the necessary directory structure under NETFLOP_ROOT
#  if it's not there already
#
for DIR in etc/conf/mod.d etc/conf/bin usr/sbin usr/lib usr/bin bin nics xtra
do
        [ ! -d "${NETFLOP_ROOT}/${DIR}" ] && {
                mkdir -p ${NETFLOP_ROOT}/${DIR}
		RET=$?
		[ ${RET} != 0 ] && {
			echo "Couldn't create directory ${NETFLOP_ROOT}/${DIR}."
			exit 1
		}

        }
done

#
#  Zip up the stuff we need for 3rd parties to make their own 
#  network install floppies
#
cp ${PROTO}/bin/bzip ${NETFLOP_ROOT}/xtra
cp ${PROTO}/bin/wrt ${NETFLOP_ROOT}/xtra
cp ${PROTO}/desktop/buildscripts/cpioout ${NETFLOP_ROOT}/xtra
cd ${NETFLOP_ROOT}/xtra
find . -print | sh ${PROTO}/desktop/buildscripts/cpioout > \
	${NETFLOP_ROOT}/xtra.z

#
#  Since the config files for the nics pkg get translated, the config
#  files are on the boot floppy.  This creates an "empty" config.z on
#  the net install floppy, so that the net install floppy can still be
#  modified to add new drivers.  The netinst script will read both.
#
mkdir -p ${NETFLOP_ROOT}/nics/config
NICLOC="${NETFLOP_ROOT}/nics"
cd ${NICLOC}
find config -print | sh ${PROTO}/desktop/buildscripts/cpioout > \
	${NETFLOP_ROOT}/nics/config.z

> ${NETFLOP_ROOT}/etc/conf/mod_register

#
#  sdev_list is used by netinst script so that the DCU can be updated
#  with the selected networking hardware driver
#
> ${NETFLOP_ROOT}/etc/conf/sdev_list
for MODULE in ${NIC_DRIVERS}
do
	grep "^${MODULE}	" ${ROOT}/${MACH}/etc/conf/sdevice.d/${MODULE} \
		>> ${NETFLOP_ROOT}/etc/conf/sdev_list
done

#
#  Copy some things from the tree that we need to modify in this script
#
for CONFIG in protocols services strcf
do
        [ ! -f ${ROOT}/${MACH}/etc/inet/${CONFIG} ] && {
                echo "ERROR: ${ROOT}/${MACH}/etc/inet/${CONFIG} not found"
                exit 1
        }
        cp ${ROOT}/${MACH}/etc/inet/${CONFIG} ${NETFLOP_ROOT}/etc/
done

#
#  Write an identifiable string into an id file so that netinst
#  knows this is the right floppy!
#
echo "${REL_FULLNAME}" > ${NETFLOP_ROOT}/id

#
#  Append "ii_boot" clause to strcf; it's the same as boot but does not
#  include a 'rawip' interface, which we don't need for Net Install.
#
cat >> ${NETFLOP_ROOT}/etc/strcf << EONET
ii_boot {
        #
        # queue params
        #
        initqp /dev/ip muxrq 40960 64386 rq 8192 40960
        initqp /dev/tcp muxrq 8192 40960
        initqp /dev/udp hdrq 32768 64512
        #
        # transport
        #
        tp /dev/tcp
        tp /dev/udp
        tp /dev/icmp
}

EONET

cat > ${NETFLOP_ROOT}/etc/netconfig << EONET
ticlts	   tpi_clts	  v	loopback	-	/dev/ticlts	/usr/lib/straddr.so
ticots	   tpi_cots	  v	loopback	-	/dev/ticots	/usr/lib/straddr.so
ticotsord  tpi_cots_ord	  v	loopback	-	/dev/ticotsord	/usr/lib/straddr.so
tcp	tpi_cots_ord	v	inet	tcp	/dev/tcp	/usr/lib/tcpip.so,/usr/lib/resolv.so
udp	tpi_clts  	v	inet	udp	/dev/udp	/usr/lib/tcpip.so,/usr/lib/resolv.so
icmp	tpi_raw  	-	inet	icmp	/dev/icmp	/usr/lib/tcpip.so,/usr/lib/resolv.so
rawip	tpi_raw  	-	inet	-	/dev/rawip	/usr/lib/tcpip.so,/usr/lib/resolv.so
ipx tpi_clts     v netware ipx /dev/ipx   /usr/lib/novell.so
spx tpi_cots_ord v netware spx /dev/nspx2 /usr/lib/novell.so
EONET

#
#  Assemble a cpio archive of message catalogs for SPX.  (The SPX commands
#  will not run if message catalogs are not in place)
#
cd ${ROOT}/${MACH}
sh ${PROTO}/desktop/buildscripts/cpioout > \
	${NETFLOP_ROOT}/usr/lib/msgcat.cpio.z << EONET
usr/lib/locale/C/LC_MESSAGES/npsmsgs.cat
usr/lib/locale/C/LC_MESSAGES/npsmsgs.cat.m
usr/lib/locale/C/LC_MESSAGES/utilmsgs.cat
usr/lib/locale/C/LC_MESSAGES/utilmsgs.cat.m
usr/lib/locale/C/LC_MESSAGES/nwcmmsgs.cat
usr/lib/locale/C/LC_MESSAGES/nwcmmsgs.cat.m
EONET

cd ${ROOT}/${MACH}/sbin
echo "resmgr" | sh ${PROTO}/desktop/buildscripts/cpioout > \
	${NETFLOP_ROOT}/resmgr.z

#
#  We need to build all of the loadable modules that go on the
#  Network Installation Utilities floppy.  Here we construct one
#  idbuild command line to build all of the drivers that do not
#  driver hardware (anything that's not a NIC driver).
#  This command line will contain a list of all drivers that are
#  older than the Driver.o in the integration tree.
#
COMMAND_LINE=""
for DRV in ${TCP_DRIVERS} ${SPX_DRIVERS} ${NET_DRIVERS}
do
	if [ -f ${ROOT}/.${MACH}/etc/conf/mod.d/${DRV} ]
	then
		if [ ${ROOT}/.${MACH}/etc/conf/pack.d/${DRV}/Driver.o \
			-nt ${ROOT}/.${MACH}/etc/conf/mod.d/${DRV} ]
		then
			COMMAND_LINE="${COMMAND_LINE}-M ${DRV} "
			switch_driver ${DRV} turnon
		fi
	else
		COMMAND_LINE="${COMMAND_LINE}-M ${DRV} "
		switch_driver ${DRV} turnon
	fi
done

#
#  Build the drivers (if there are any to build)
#
if [ ! -z "${COMMAND_LINE}" ]
then
	MACH=.${MACH} idbuild -c ${COMMAND_LINE}
fi

#
#  Turn all of the drivers back off, and update mod_register so that
#  we can load the drivers on the machine doing the network install.
#
for DRV in ${TCP_DRIVERS} ${SPX_DRIVERS} ${NET_DRIVERS}
do
	mod_reg_updt ${DRV} ${ROOT}/.${MACH}/etc/conf/mod.d
	cp ${ROOT}/.${MACH}/etc/conf/mod.d/${DRV} ${NETFLOP_ROOT}/etc/conf/mod.d
	switch_driver ${DRV} turnoff
done
		

#
#  For each of the Networking card drivers, we do the following:
#	turn it on (in the sdevice file)
#	idbuild -c -M it
#	update mod_register for it
#	turn it off (in the sdevice file)
#	compress it into the target directory
#
#  Just for a belt and suspenders, make sure all NIC drivers are turned off.
#
echo "Turning off all networking drivers"
for DRV in ${NIC_DRIVERS}
do
	switch_driver ${DRV} turnoff
done

#
#  Now build the NIC drivers
#  The only drivers that will actually get built are those where the
#  driver is older than the Driver.o in the integration tree.
#
echo "Building networking drivers"
for DRV in ${NIC_DRIVERS}
do
	switch_driver ${DRV} turnonnic
	if [ -f ${ROOT}/.${MACH}/etc/conf/mod.d/${DRV} ]
	then
		if [ ${ROOT}/.${MACH}/etc/conf/pack.d/${DRV}/Driver.o \
			-nt ${ROOT}/.${MACH}/etc/conf/mod.d/${DRV} ]
		then
			MACH=.${MACH} idbuild -c -M ${DRV}
		fi
	else
		MACH=.${MACH} idbuild -c -M ${DRV}
	fi

	#
	#  This creates a small subtree for each driver containing the
	#  files the driver needs, and then creates a single cpio archive
	#  of that subtree for each driver.
	#
	mkdir -p ${NETFLOP_ROOT}/nics/${DRV}
		#
		#  Any "extra" files first (including drvmap file)
		#
	cd ${ROOT}/${MACH}
	cat ${IITMP}/${DRV}.files | \
		cpio -pdumv ${NETFLOP_ROOT}/nics/${DRV} 2> /dev/null
		#
		#  Then the driver itself
		#
	cd ${ROOT}/.${MACH}
	echo "etc/conf/mod.d/${DRV}" | cpio -pdumv ${NETFLOP_ROOT}/nics/${DRV}
		#
		#  Then assemble the driver-specific cpio archive.
		#
	cd ${NETFLOP_ROOT}/nics/${DRV}
	find . -print | sh ${PROTO}/desktop/buildscripts/cpioout > \
		${NETFLOP_ROOT}/nics/${DRV}.z

	#
	#  Now we update the netflop.proto file for building the Network
	#  Installation Utilities floppy.
	#
	echo "\t${DRV}.z ---444 2 2 ${NETFLOP_ROOT}/nics/${DRV}.z" \
		>> ${FLOPPROTO}.tmp

	#
	#  This updates mod_register so that this driver can be loaded
	#
	mod_reg_updt ${DRV} ${ROOT}/.${MACH}/etc/conf/mod.d
	switch_driver ${DRV} turnoff
done

#
#  Finish up the prototype file for the s5 filesystem on the Network
#  Installation Utilities floppy
#
echo "\t$" >> ${FLOPPROTO}.tmp
echo "$" >> ${FLOPPROTO}.tmp

#
#  Get executables/libes config files out of $ROOT/$MACH and stage in
#  net.flop area for cpio into compressed archives.
#
cd ${ROOT}/${MACH}
cpio -pd ${PROTO}/net.flop <<EOF
etc/conf/bin/idmodreg
etc/netware/conf/nwnet.bin
usr/sbin/pkgcat
usr/sbin/mknod
usr/sbin/npsd
usr/sbin/nwcm
usr/sbin/nwdiscover
usr/sbin/bootp
usr/sbin/ifconfig
usr/sbin/ping
usr/sbin/route
usr/sbin/slink
usr/lib/libNwCal.so
usr/lib/libNwClnt.so
usr/lib/libNwLoc.so
usr/lib/libNwNcp.so
usr/lib/libcrypt.so
usr/lib/libiaf.so
usr/lib/libnsl.so
usr/lib/libnwnetval.so
usr/lib/libsocket.so
usr/lib/libthread.so
usr/lib/novell.so
usr/lib/libnwutil.so
usr/lib/libresolv.so
usr/lib/tcpip.so
usr/bin/dd
usr/bin/uname
EOF
cp ${PROTO}/cmd/sap_nearest ${PROTO}/net.flop/bin/

#
#  Create compressed archives for inclusion on Network Installation
#  Utilities floppy - common, spx, tcp
#
cd $PROTO/net.flop
PATH=$PROTO/cmd:$PATH export PATH

echo | sh ${PROTO}/desktop/buildscripts/cpioout > common.z << EONET
usr/bin/dd
usr/bin/uname
usr/lib/libcrypt.so
usr/lib/libiaf.so
usr/lib/libnsl.so
usr/lib/libsocket.so
usr/lib/libthread.so
usr/sbin/pkgcat
usr/sbin/mknod
etc/netconfig
etc/services
etc/conf/mod_register
etc/conf/sdev_list
etc/conf/bin/idmodreg
etc/conf/mod.d/ethtsm
etc/conf/mod.d/lsl
etc/conf/mod.d/msm
etc/conf/mod.d/net
etc/conf/mod.d/odisr
etc/conf/mod.d/sockmod
etc/conf/mod.d/timod
etc/conf/mod.d/tirdwr
etc/conf/mod.d/toktsm
EONET

echo | sh ${PROTO}/desktop/buildscripts/cpioout > spx.z << EONET
bin/sap_nearest
usr/sbin/npsd
usr/sbin/nwcm
usr/sbin/nwdiscover
usr/lib/libNwCal.so
usr/lib/libNwClnt.so
usr/lib/libNwLoc.so
usr/lib/libNwNcp.so
usr/lib/libnwnetval.so
usr/lib/libnwutil.so
usr/lib/msgcat.cpio.z
usr/lib/novell.so
etc/conf/mod.d/ipx
etc/conf/mod.d/nspx
etc/conf/mod.d/ripx
etc/conf/mod.d/uni
etc/netware/conf/nwnet.bin
EONET

echo | sh ${PROTO}/desktop/buildscripts/cpioout > tcp.z << EONET
usr/lib/libresolv.so
usr/lib/tcpip.so
usr/sbin/bootp
usr/sbin/ifconfig
usr/sbin/ping
usr/sbin/route
usr/sbin/slink
etc/strcf
etc/conf/mod.d/app
etc/conf/mod.d/arp
etc/conf/mod.d/icmp
etc/conf/mod.d/inet
etc/conf/mod.d/ip
etc/conf/mod.d/route
etc/conf/mod.d/tcp
etc/conf/mod.d/udp
EONET

#
#  Code to actually cut the floppy.
#
ask_drive 2> $PROTO/stage/drive_info

echo "\nInsert Net Install Floppy into $MEDIUM drive and"
echo "press <RETURN>, s to skip, or F to first format: \c"
read a
[ "$a" != "s" ] && {
	if [ "$a" = "F" ] 
	then
		/usr/sbin/format /dev/rdsk/${FDRIVE}t || exit $?
	fi
	grep -v "^#" ${FLOPPROTO}.tmp |
		sed -e "s,\$ROOT,$ROOT," \
		-e "s,\$MACH,$MACH," \
		-e "s,\$WORK,$WORK," \
		> $FLOPPROTO
        touch ${PROTO}/stage/netflop.image $FLOPPROTO
        /sbin/mkfs -Fs5 -b 512 ${PROTO}/stage/netflop.image $FLOPPROTO
        echo mkfs returns $?
        dd if=${PROTO}/stage/netflop.image of=/dev/dsk/${FDRIVE}t bs=36b
        echo dd returns $?
}

rm -rf ${IITMP}
