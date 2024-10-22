#!/usr/bin/ksh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:cmd/cut.flop.sh	1.1.1.40"

# script to create boot floppy.

function ask_drive
{
	[ -n "$MEDIUM" ] ||
	read "MEDIUM?Please enter diskette1 or diskette2 (default is diskette1): "
	[ -n "$MEDIUM" ] || MEDIUM="diskette1"
	case "$MEDIUM" in
	diskette1)
		DRVLETTER=A
		;;
	diskette2)
		DRVLETTER=B
		;;
	*)
		print -u2 ERROR: Must specify diskette1 or diskette2.
		exit 1
		;;
	esac
	export BLOCKS=$(devattr $MEDIUM capacity)
	case $BLOCKS in
	2844) # 3.5-inch diskette
		TRKSIZE=36
		if [ "${LANG}" = "ja" ]
		then
			FIRST='FIRST '
		else
			FIRST=''
		fi
		;;
	2370) # 5.25-inch diskette
		TRKSIZE=30
		FIRST='FIRST '
		;;
	*)
		print -u2 ERROR -- diskette must be either 1.44MB 3.5 inch
		print -u2 or 1.2MB 5.25 inch
		exit 2
		;;
	esac
	FDRIVE=$(devattr $MEDIUM fmtcmd)
	FDRIVE=${FDRIVE##* }
}

function select_boot
{
	rm -f boot fboot

	BOOT=$PROTO/locale/$LANG/boot.fd
	[ -s "$BOOT" ] || BOOT=$PROTO/locale/C/boot.fd
	[ -s "$BOOT" ] || {
		print -u2 ERROR: $BOOT does not exist.
		exit 1
	}
	ln -s $BOOT boot

	FBOOT=$ROOT/$LCL_MACH/etc/fboot
	[ -z "$special_flag" ] || FBOOT=$ROOT/$LCL_MACH/etc/cpqfboot
	[ -s "$FBOOT" ] || {
		print -u2 ERROR: $FBOOT does not exist.
		exit 1
	}
	ln -s $FBOOT fboot
}

function stripem
{
	typeset i save_pwd=$PWD

	[ -d $2 ] || mkdir -p $2
	cd $1
	for i in *
	do
		rmwhite $i $2/$i
	done
	cd $save_pwd
}

function strip_comments
{
	. $PROTO/bin/rmwhite
	[ -d $PROTO/stage/winxksh ] || mkdir $PROTO/stage/winxksh
	rmwhite $ROOT/$MACH/usr/lib/winxksh/scr_init $PROTO/stage/winxksh/scr_init
	rmwhite $ROOT/$MACH/usr/lib/winxksh/winrc $PROTO/stage/winxksh/winrc

	stripem $ROOT/$MACH/etc/dcu.d/menus   $PROTO/stage/dcu.d/menus
	stripem $ROOT/$MACH/etc/dcu.d/scripts $PROTO/stage/dcu.d/scripts
	stripem $PROTO/desktop/menus          $PROTO/stage/desktop/menus
	stripem $PROTO/desktop/scripts        $PROTO/stage/desktop/scripts
}

function prep_flop2
{
	(( BLOCKS == 2370 )) || [ "$LANG" = "ja" ] || return
	print "Making tree for SECOND boot floppy."
	rm -rf flop2root
	set -e
	mkdir flop2root
	cd flop2root
	mkdir -p \
		etc/conf/hbamod.d \
		etc/dcu.d/dculib \
		etc/dcu.d/locale/${LANG} \
		etc/dcu.d/menus \
		etc/dcu.d/scripts \
		etc/inst/scripts \
		etc/inst/locale/C/menus/help \
		etc/scsi \
		sbin \
		usr/bin \
		usr/sbin
	cp $PROTO/locale/$LANG/menus/help/nics.conf/config.z etc/inst/locale/C/menus/help/config.z
	cp $PROTO/locale/$LANG/menus/help/nicshlp.z etc/inst/locale/C/menus/help/nicshlp.z
	cp $PROTO/stage/dcu.d/menus/drivers etc/dcu.d/menus/drivers
	cp $PROTO/stage/dcu.d/scripts/dculib.sh etc/dcu.d/scripts/dculib.sh
	cp $PROTO/stage/desktop/menus/allinit etc/inst/scripts/allinit
	cp $PROTO/stage/desktop/menus/asktime etc/inst/scripts/asktime
	cp $PROTO/stage/desktop/menus/fd etc/inst/scripts/fd
	cp $PROTO/stage/desktop/menus/fdinit etc/inst/scripts/fdinit
	cp $PROTO/stage/desktop/menus/floppy2 etc/inst/scripts/floppy2
	cp $PROTO/stage/desktop/menus/fs etc/inst/scripts/fs
	cp $PROTO/stage/desktop/menus/ii_hw_config etc/inst/scripts/ii_hw_config
	cp $PROTO/stage/desktop/menus/ii_hw_select etc/inst/scripts/ii_hw_select
	cp $PROTO/stage/desktop/menus/nond_init etc/inst/scripts/nond_init
	cp $PROTO/stage/desktop/menus/pkgs etc/inst/scripts/pkgs
	cp $PROTO/stage/desktop/menus/useanswers etc/inst/scripts/useanswers
	cp $PROTO/stage/desktop/scripts/desktop.prep etc/inst/scripts/desktop.prep
	cp $PROTO/stage/desktop/scripts/netinst etc/inst/scripts/netinst
	cp $ROOT/$MACH/etc/dcu.d/dculib/dculib.so etc/dcu.d/dculib/dculib.so
	cp $ROOT/$MACH/etc/dcu.d/locale/$LANG/txtstrings etc/dcu.d/locale/$LANG/txtstrings
	cp $ROOT/$MACH/etc/scsi/bmkdev etc/scsi/bmkdev
	cp $ROOT/$MACH/usr/bin/ls usr/bin/ls
	cp $ROOT/$MACH/usr/bin/uncompress usr/bin/uncompress
	cp $ROOT/$MACH/usr/sbin/fdisk.boot usr/sbin/fdisk
	cp $ROOT/$MACH/usr/sbin/prtvtoc usr/sbin/prtvtoc
	cp $ROOT/.$MACH/etc/conf/modnew.d/adsc etc/conf/hbamod.d/adsc
	cp $ROOT/.$MACH/etc/conf/modnew.d/cpqsc etc/conf/hbamod.d/cpqsc
	cp $ROOT/.$MACH/etc/conf/modnew.d/dpt etc/conf/hbamod.d/dpt
	cp $ROOT/.$MACH/etc/conf/modnew.d/ictha etc/conf/hbamod.d/ictha
	set +e
	LIST=$(find * -type f -print)
	print $LIST | xargs chmod 555 
	print $LIST | xargs chgrp sys
	print $LIST | xargs chown root
	typeset OIFS=$IFS
	IFS=
	print -r $LIST | cpio -oLDV -H crc > /tmp/out1.$$
	IFS=$OIFS
	cd ..
	print Compressing image for SECOND boot floppy.
	bzip -s32k /tmp/out1.$$ > /tmp/out2.$$
	wrt -s /tmp/out2.$$ > flop2.image
	rm /tmp/out?.$$
}

function get_answer
{
	while :
	do
		print -n "\007\nInsert $1 floppy into $MEDIUM drive and press\n\t<ENTER> "
		print -n "to write floppy,\n\tF\tto format and write floppy,\n\ts\tto "
		print -n "skip, or\n\tq\tto quit: "
		read a
		case "$a" in
		"")
			return 0
			;;
		F)
			/usr/sbin/format -i$2 $FDRIVE || exit $?
			return 0
			;;
		s)
			return 1
			;;
		q)
			exit 0
			;;
		*)
			print -u2 ERROR: Invalid response -- try again.
			;;
		esac
	done
}

function cut_flop1
{
	get_answer "${FIRST}boot" 1 || return
	sbfpack fboot dcmp mip sip boot $LOGO_IMG unix resmgr $FDRIVE || exit $?
	if [ -z "$SERIAL" ]
	then
		print -u2 "INFO: floppy being stamped with blanks"
	fi
	hsflop -o -w $DRVLETTER "$SERIAL" || exit $?  #write serial_data
	print "Done with ${FIRST}boot floppy.\007"
}

function cut_flop2
{
	integer count
	(( BLOCKS == 2370 )) || [ "$LANG" = "ja" ] || return
	get_answer "SECOND boot" 2 || return
	print Writing to SECOND boot floppy.
	set -- $(ls -l flop2.image)
	(( count = $5 / 512 ))
	/usr/bin/dd if=flop2.image of=$FDRIVE bs=${TRKSIZE}b || exit $?
	. ${ROOT}/${MACH}/var/sadm/dist/rel_fullname
	hsflop -o -w $DRVLETTER "${REL_FULLNAME} Boot Floppy 2" || exit $?
	print "Wrote $count blocks to SECOND boot floppy."
	print "Done with SECOND boot floppy.\007"
}

function cut_magic
{
	get_answer "magic" 2 || return
	grep -v "^#" $PROTO/desktop/files/magic.proto |
		sed \
			-e "s,\$ROOT,$ROOT," \
			-e "s,\$MACH,$MACH," \
			-e "s,\$WORK,$WORK," \
			> $FLOPPROTO
	> $MAGICIMAGE
	/sbin/mkfs -Fs5 -b 512 $MAGICIMAGE $FLOPPROTO 2 $TRKSIZE || exit $?
	/usr/bin/dd if=$MAGICIMAGE of=$FDRIVE bs=${TRKSIZE}b || exit $?
	print "Done with magic floppy.\007"
}

#main()

FLOPPROTO=/tmp/flopproto$$
MAGICIMAGE=/tmp/magic$$
trap "rm -f $FLOPPROTO $MAGICIMAGE; exit" 0 1 2 3 15
PATH=$PROTO/bin:$PATH export PATH
LOGO_IMG=logo.img
setflag=-a		#default is AS set
LANG=C
unset special_flag
nflag=0
quick=false

while getopts qpal:sn: c
do
	case $c in
		p)
			# make PE floppy
			setflag=-p
			;;
		a)
			# make AS floppy (default)
			;;
		l)
			LANG=$OPTARG
			;;
		s)	
			special_flag=-s
			;;
		n)
			nflag=1
			SERIAL=$OPTARG
			;;
		q)
			quick=true
			;;
		\?)
			print -u2 "Usage: $0 -n serial_data [-a|-p] [-l locale] [diskette1|diskette2]"
			print -u2 "\t-n specifies a (serial ID, activation key pair)."
			print -u2 "\t-a cuts an AS boot floppy."
			print -u2 "\t-p cuts a PE boot floppy."
			print -u2 "\t-l specifies the locale for the boot floppy."
			exit 1
			# The -s and -q options are intentionally not listed here.
			;;
		*)
			print -u2 Internal error during getopts.
			exit 2
			;;
	esac
done
(( VAL = OPTIND - 1 ))
shift $VAL

(( nflag )) || {
	print -u2 "must use -n option"
	exit 2
}

MEDIUM=$1
if [ "$PROTO" = "" ]
then
	if [ "$2" = "" ]
	then
		read "PROTO?PROTO is not set. Enter the path for PROTO: "
	else
		PROTO=$2
	fi
fi
if [ "$ROOT" = "" ]
then
	if [ "$3" = "" ]
	then
		read "ROOT?ROOT is not set. Enter the path for ROOT: "
	else
		ROOT=$3
	fi
fi
if [ "$MACH" = "" ]
then
	if [ "$4" = "" ]
	then
		read "MACH?MACH is not set. Enter the path for MACH: "
	else
		MACH=$4
	fi
fi
export LCL_MACH=.$MACH

cd $PROTO/stage
ask_drive
$quick || {
	select_boot
	/usr/bin/cat $PROTO/locale/*/smartmsg1 > smartmsg1

	>lang.items
	>lang.msgs
	>lang.footers
	for i in $PROTO/locale/*
	do
		if [ -f $i/lang.msgs -a -f $i/lang.footers ]
		then
			echo `basename $i` >> lang.items
			cat $i/lang.msgs >> lang.msgs
			cat $i/lang.footers >> lang.footers
		fi
	done

	/usr/bin/egrep '(^PC850.*88591.*d$)|(^sjis.*eucJP)' \
		$ROOT/$MACH/var/opt/ls/iconv_data.ls > iconv_data

	#
	# We need the lp drvmap file on the boot floppy, but its
	# verify routine is NOT available to be run from the boot
	# floppy.  So the user doesn't get a "failed to verify"
	# message, we'll indicate NO verify routine during install.
	#

	ed -s $ROOT/.$MACH/etc/conf/drvmap.d/lp <<-EOF
	g/Y|Y/s//Y|N/
	w
	q
	EOF

	strip_comments
	conframdfs $special_flag $setflag -l $LANG || exit $?
	make -f $PROTO/desktop/cut.flop.mk || exit $?
	prep_flop2
}
cut_flop1
cut_flop2
cut_magic
