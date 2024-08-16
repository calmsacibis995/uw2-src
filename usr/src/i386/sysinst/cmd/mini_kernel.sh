#!/usr/bin/ksh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:cmd/mini_kernel.sh	1.1.2.41"

turnoff()
{
	cd $ROOT/$MACH/etc/conf/sdevice.d
	[ -d .save ] || mkdir .save
	mv * .save
	cd $ROOT/$MACH/etc/conf/mdevice.d
	[ -d .save ] || mkdir .save
	mv * .save
}

turnon()
{
	FAIL=false
	cd $ROOT/$MACH/etc/conf/sdevice.d/.save
	for i
	do
		[ -f $i ] || {
			print -u2 "$CMD: Fatal error -- cannot find kernel module $i."
			FAIL=true
		}
	done
	$FAIL && exit 2
	mv $* ..
	cd $ROOT/$MACH/etc/conf/sdevice.d
	for i in $(grep -l '[	 ]N[	 ]' $*)
	do
		# Some System files (like asyc's) have both Y and N entries.
		# Do not edit such files.
		case "$(print $(<$i))" in
		*' Y '*)
			continue
			;;
		esac
		ed -s $i <<-END
			g/[	 ]N[	 ]/s//	Y	/
			w
			q
		END
	done
	cd $ROOT/$MACH/etc/conf/mdevice.d/.save
	mv $* ..
}

stub()
{
	FAIL=false
	cd $ROOT/$MACH/etc/conf/sdevice.d/.save
	for i
	do
		[ -f $i ] || {
			print -u2 "$CMD: Fatal error -- cannot find kernel module $i."
			FAIL=true
		}
	done
	$FAIL && exit 2
	mv $* ..
	cd $ROOT/$MACH/etc/conf/sdevice.d
	for i in $(grep -l '[	 ]Y[	 ]' $*)
	do
		ed -s $i <<-END
			g/[	 ]Y[	 ]/s//	N	/
			w
			q
		END
	done
	cd $ROOT/$MACH/etc/conf/mdevice.d/.save
	mv $* ..
}

make_static()
{
	FAIL=false
	cd $ROOT/$MACH/etc/conf/sdevice.d
	for i
	do
		case "$(print $(<$i))" in
		*'$version 2 $static'*)
			continue
			;;
		*'$version 2'*)
			;;
		*)
			print -u2 "$CMD: Fatal error -- kernel module $i is not version 2."
			FAIL=true
			continue
			;;
		esac
		ed -s $i <<-END
			/\$version/a
			\$static
			.
			w
			q
		END
	done
	$FAIL && exit 2
}

tune()
{
	idtune -f PHYSTOKVMEM 1
	idtune -f PAGES_UNLOCK 50
	idtune -f FLCKREC 100
	idtune -f NPROC 100
	idtune -f MAXUP 30
	idtune -f NHBUF 32
	idtune -f FD_DOOR_SENSE 0
	idtune -f ARG_MAX 20480 # Need large space for exported env variables
}

#main()

CMD=$0
PATH=$ROOT/$MACH/etc/conf/bin:$PATH
USAGE="Usage: $0 kdb|nokdb"

STATIC_LIST="ansi asyc atup ca ccnv cdfs char clone cmux confmgr cram dcompat
dma eisa elf fd fdbuf fifofs fpe fs gentty gvid iaf iasy intmap intp io
kd kdvm kernel kma ldterm mca mem memfs mm mod modksym name namefs nullzero pci
pit proc procfs pstart ramd resmgr rtc s5 sad sc01 sd01 sdi specfs st01 sum svc
sysclass ts udev util ws"

if [ "$LANG" = "ja" ]
then
	STATIC_LIST="$STATIC_LIST gsd fnt"
fi

FS_LIST="bfs dosfs dow sfs ufs vxfs"
HBA_LIST="adsc cpqsc dpt ictha"
DCD_LIST="dcd athd mcesdi mcst"
NET_LIST="net sockmod timod tirdwr"
INET_LIST="app arp icmp inet ip route tcp udp"
IPX_LIST="ipx nspx ripx uni"
ODI_LIST="lsl msm ethtsm toktsm odisr"

DYNAMIC_LIST="$FS_LIST $HBA_LIST $DCD_LIST
$NET_LIST $INET_LIST $IPX_LIST $ODI_LIST"

STUB_LIST="asyhp async audit coff dac event ipc log mac merge nfs prf pse rand
segdev sysdump tpath xnamfs xque"

(( $# == 1 )) || {
	print -u2 $USAGE
	exit 2
}

case "$1" in
kdb)
	STATIC_LIST="$STATIC_LIST kdb kdb_util"
	print > $ROOT/$MACH/etc/conf/cf.d/kdb.rc
	;;
nokdb)
	DYNAMIC_LIST="$DYNAMIC_LIST kdb kdb_util"
	rm -f $ROOT/$MACH/etc/conf/cf.d/kdb.rc
	;;
*)
	print -u2 $USAGE
	exit 2
	;;
esac

turnoff
turnon $STATIC_LIST $DYNAMIC_LIST
stub $STUB_LIST
make_static $STATIC_LIST
tune
print $DYNAMIC_LIST
exit 0
