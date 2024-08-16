#!/usr/bin/ksh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)proto:cmd/conframdfs.sh	1.1.1.33"

PATH=$PROTO/bin:$TOOLS/usr/ccs/bin:$PATH: export PATH
EDSYM="bin/edsym"
NM="${PFX}nm"
DUMP="${PFX}dump"
STRIP="${PFX}strip"
UNIXSYMS="${PFX}unixsyms"
MCS="${PFX}mcs"

setflag=-a		#default is AS set
LANG=C
special_flag=false
mv2cd_flag=false
while getopts pal:s c
do
	case $c in
		p)
			# make PE floppy
			setflag=-p
			;;
		a)
			# make AS floppy
			setflag=-a
			;;
		l)
			LANG=$OPTARG
			;;
		s)
			special_flag=true
			mv2cd_flag=true
			;;
		\?)
			print -u2 "Usage: $0 [-a|-p] [-l locale]"
			# The -s option is intentionally not listed here.
			exit 1
			;;
		*)
			print -u2 Internal error during getopts.
			exit 2
			;;
	esac
done

LCL_MACH=.$MACH
BASE=$ROOT/$LCL_MACH
SOURCE_KERNEL=$BASE/stand/unix.nostrip
DEST_KERNEL=$BASE/stand/unix

# Begin main processing
print "\nWorking in locale/$LANG/menus directory."
cd $PROTO
[ -d locale/$LANG/menus/help ] || {
	print -u2 ERROR -- locale/$LANG/menus/help directory does not exist.
	exit 2
}
(cd locale/$LANG/menus/help
 make -f help.mk all
 ls *.hcf | ksh ${PROTO}/desktop/buildscripts/cpioout > \
 locale_hcf.z || {
	print -u2 ERROR -- could not create locale_hcf.z.
	exit 2
    }
) 
(cd locale/$LANG/menus; $ROOT/$MACH/usr/lib/winxksh/compile config.sh)

(
	PATH=$PROTO/bin:$PROTO/cmd:$PATH export PATH
	if [ "${LANG}" = "C" ]
	then
		NICLOC="${ROOT}/usr/src/${WORK}/cmd/cmd-nics"
		SUPPHELP=.
	else
		NICLOC="${ROOT}/${MACH}/etc/inst/locale/${LANG}/menus/nics"
		SUPPHELP=help
	fi
	rm -rf $PROTO/locale/$LANG/menus/help/nics.d &&
	    mkdir -p $PROTO/locale/$LANG/menus/help/nics.d
	cp ${NICLOC}/help/*hcf \
	    $PROTO/locale/$LANG/menus/help/nics.d || {
		print -u2 ERROR -- copy failed from nics help directory.
		exit 2
	    }
	cp ${NICLOC}/supported_nics/${SUPPHELP}/*hcf \
	    $PROTO/locale/$LANG/menus/help/nics.d || {
		print -u2 ERROR -- copy failed from supported_nics directory.
		exit 2
	    }
	cd $PROTO/locale/$LANG/menus/help/nics.d
	find . -print | ksh ${PROTO}/desktop/buildscripts/cpioout > \
	    $PROTO/locale/$LANG/menus/help/nicshlp.z || {
		print -u2 ERROR -- could not create nicshlp.z.
		exit 2
	    }
) || exit $?

(
	PATH=$PROTO/bin:$PROTO/cmd:$PATH export PATH
	if [ "${LANG}" = "C" ]
	then
		NICLOC="${ROOT}/usr/src/${WORK}/pkg/nics"
	else
		NICLOC="${ROOT}/${MACH}/etc/inst/locale/${LANG}/menus/nics"
	fi
	rm -rf $PROTO/locale/$LANG/menus/help/nics.conf/config &&
	    mkdir -p $PROTO/locale/$LANG/menus/help/nics.conf/config
	cp ${NICLOC}/config/* \
	    $PROTO/locale/$LANG/menus/help/nics.conf/config || {
		print -u2 ERROR -- copy failed from nics config directory.
		exit 2
	    }
	cd $PROTO/locale/$LANG/menus/help/nics.conf
	find config -print | ksh ${PROTO}/desktop/buildscripts/cpioout > \
	    $PROTO/locale/$LANG/menus/help/nics.conf/config.z || {
		print -u2 ERROR -- could not create config.z.
		exit 2
	    }
) || exit $?

[ ! -s $SOURCE_KERNEL ] && {
	print -u2 ERROR -- $SOURCE_KERNEL does not exist.
	exit 1
}
print "\nCopying $SOURCE_KERNEL into\n$DEST_KERNEL."
cp $SOURCE_KERNEL $DEST_KERNEL &

if $special_flag
then
	sed -e '/:dcd:/d' $BASE/stand/loadmods > $PROTO/stage/loadmods
else
	cp $BASE/stand/loadmods $PROTO/stage/loadmods
fi

pick.set $setflag -l $LANG || exit $?

sizeline=""
sizeline=`$NM -px $SOURCE_KERNEL | grep RootRamDiskBuffer`
set $sizeline

if [ "$3" != "RootRamDiskBuffer" ]
then
	print -u2 "Cannot find symbol RootRamDiskBuffer in file $SOURCE_KERNEL."
	exit 1
fi
bufferaddr=$1

RAMPROTO="desktop/files/ramdfs.proto"
LCL_TEMP=/tmp/ramd$$ export LCL_TEMP
ROOTFS=$LCL_TEMP/ramdisk.fs

trap "rm -rf $LCL_TEMP; exit" 1 2 3 15
mkdir $LCL_TEMP

#
# Japanese requires two floppies, regardless of the floppy size
#
if (( BLOCKS == 2844 )) && [ "$LANG" != "ja" ]
then
	# uncomment lines for files on 3.5-inch floppy
	FLOP2="-e s,^#flop2,,"
else
	# leave lines commented out for 5.25-inch floppy
	FLOP2=""
fi
	
SMARTCMD='-e \,desktop/menus/smart,d'
$special_flag && [ -s $PROTO/desktop/menus/smart ] && SMARTCMD=""

# Compaq specific - lines in randfs.proto beginning with #mv2cd indicate
# files that will appear on the CD instead of on the boot floppy image
MV2CD="-e s,^#mv2cd,,"
$mv2cd_flag && MV2CD=""

#Uncomment all locale-specific lines, if any
#Delete all other comment lines
# Note: $MV2CD must be executed by sed before $FLOP2 is executed
sed \
	-e "s,^#$LANG,," \
	$MV2CD \
	$FLOP2 \
	-e '/^#/d' \
	$SMARTCMD \
	-e "s,\$ROOT,$ROOT," \
	-e "s,\$MACH,$MACH," \
	-e "s,\$WORK,$WORK," \
	-e "s,\$LANG,$LANG," \
	$RAMPROTO \
	> $LCL_TEMP/ramdproto

> $ROOTFS
print "\nMaking file system image.\n"
/sbin/mkfs -Fs5 -b 512 $ROOTFS $LCL_TEMP/ramdproto 2 36 2>$LCL_TEMP/mkfs.log
if [ -s $LCL_TEMP/mkfs.log ]
then
	print -u2 "\nERROR -- mkfs of ramdisk filesystem failed."
	cat $LCL_TEMP/mkfs.log >&2
	print -u2 "\nErrors are logged in $LCL_TEMP/mkfs.log"
	rm $ROOTFS
	exit 1
fi

wait # wait for cp into $DEST_KERNEL to finish
unset FIRST_DATA
$DUMP -hv $DEST_KERNEL | grep '\.data$' |
while read x1 x2 x3 section offset x4 name
do
	if [ -z "$FIRST_DATA" ]
	then
		FIRST_DATA="found"
		continue # skip the first data section
	else
		print "\nWriting the file system image into the kernel .data section."
		print "Address of RootRamDiskBuffer is $bufferaddr."
		$EDSYM -f $ROOTFS $section $offset $bufferaddr $DEST_KERNEL || {
			print -u2 "\nERROR -- Cannot find a .data setion in $DEST_KERNEL."
			rm -rf $LCL_TEMP
			exit 1
		}
		break
	fi
done
print "\nStripping symbol table from $DEST_KERNEL."
$STRIP $DEST_KERNEL
print "\nEmptying .comment section of $DEST_KERNEL."
$MCS -d $DEST_KERNEL
rm -rf $LCL_TEMP
exit 0
