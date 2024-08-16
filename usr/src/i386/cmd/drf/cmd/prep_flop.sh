#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)drf:cmd/prep_flop.sh	1.30"

set_mod_list()
{
	#Get the file system type for root, /stand(?), /usr and /home.
	#Reconsider to get only for /stand and root.

	FSCKS=`egrep '[	]/[	]|[	]/stand[	]|[	]/usr[	]|[	]/home[	]|[	]/home2[	]' /etc/vfstab | cut -f4  | egrep 'bfs|s5|ufs|vxfs' | sort -u`

	FSMODS=`echo "$FSCKS" | sed 's/s5//'`
	# ufs module depends on sfs module. So,....
	echo $FSMODS | grep ufs >/dev/null 2>&1
	[ $? -eq 0 ] && FSMODS="$FSMODS sfs"
	#Add dow module if ufs or sfs module is present
	echo $FSMODS | egrep 'ufs' > /dev/null 2>&1
	[ $? -eq 0 ] && FSMODS="$FSMODS dow"
	_DRVMODS="`/etc/scsi/pdiconfig | grep '[	]Y[	]' | cut -f1 | sort -u`"
	#Add dcd module if athd, mcesdi or mcst module is present
	echo $_DRVMODS | egrep 'athd|mcesdi|mcst' > /dev/null 2>&1
	[ $? -eq 0 ] && _DRVMODS="dcd $_DRVMODS"
	MOD_LIST="$FSMODS $_DRVMODS" 
	for i in $_DRVMODS
	do
		ed -s $RAMPROTO >/dev/null <<-END
		/mod\.d
		a
		$i    ---444 0 3 \$ROOT/.\$MACH/etc/conf/modnew.d/$i
		.
		w
		q
		END
	done

}

add_terminfo_ie()
{
	ed -s $RAMPROTO >/dev/null <<-END
	/ANSI
	a
	AT386-ie  ---644 0 3 /usr/share/lib/terminfo/A/AT386-ie
	AT386-M-ie  ---644 0 3 /usr/share/lib/terminfo/A/AT386-M-ie
	.
	w
	q
	END
	return;
}

add_terminfo_mb()
{
	ed -s $RAMPROTO >/dev/null <<-END
	/ANSI
	a
	AT386-mb  ---644 0 3 /usr/share/lib/terminfo/A/AT386-mb
	AT386-M-mb  ---644 0 3 /usr/share/lib/terminfo/A/AT386-M-mb
	.
	w
	q
	END
	return;
}

add_terminfo()
{
	ed -s $RAMPROTO >/dev/null <<-END
	/ANSI
	a
	AT386  ---644 0 3 /usr/share/lib/terminfo/A/AT386
	AT386-M  ---644 0 3 /usr/share/lib/terminfo/A/AT386-M
	.
	w
	q
	END
	return;
}

add_keyboard()
{
	[ -s /usr/lib/keyboard/$KEYBOARD ] || {
	    echo "\n\t/usr/lib/keyboard/$KEYBOARD file is missing" >> ${DRF_LOG_FL}
	    return 
	}
	[ -s /usr/lib/mapchan/88591.dk ] || {
	    echo "\n\t/usr/lib/mapchan/88591.dk file is missing" >> ${DRF_LOG_FL}
	    return 
	}
	( cd  /usr/lib/drf
	 ./keycomp /usr/lib/keyboard/$KEYBOARD $PROTO/kbmap 
	 ./dkcomp /usr/lib/mapchan/88591.dk $PROTO/dead_keys
	)
	ed -s $RAMPROTO >/dev/null <<-END
	/keyboards
	a
	$KEYBOARD   d--755 2 2 
	kbmap  ---644 0 3 $PROTO/kbmap
	dead_keys  ---644 0 3 $PROTO/dead_keys
	\$
	.
	w
	q
	END
	return
}

add_lc_ctype()
{
	[ -s /usr/lib/locale/$LANG/LC_CTYPE ] || {
	    echo "\n\t/usr/lib/locale/$LANG/LC_CTYPE file is missing" >> ${DRF_LOG_FL}
	    return 
	}
	ed -s $RAMPROTO >/dev/null <<-END
	/locale
	a
	$LANG   d--755 2 2 
	LC_CTYPE  ---644 0 3 /usr/lib/locale/$LANG/LC_CTYPE
	\$
	.
	w
	q
	END
	return
}

turnoff()
{
	cd $ROOT/$LCL_MACH/etc/conf/sdevice.d
	[ -d .save ] || mkdir .save
	mv * .save

	cd $ROOT/$LCL_MACH/etc/conf/mdevice.d
	[ -d .save ] || mkdir .save
	mv * .save
}

turnon()
{
	cd $ROOT/$LCL_MACH/etc/conf/sdevice.d/.save
	mv $* .. >/dev/null 2>&1
	cd ..

	for i in `grep -l '[	 ]N[	 ]' $* 2>/dev/null`
	do
	   # Check whether it is hardware module. If it is, leave
	   # it as it is.
	   A=`grep "^$i	" /etc/conf/mdevice.d/$i | cut -f3 | grep h`
	   if [ -z "$A" ]
	   then
		# it is not a hardware module
		ed -s $i <<-END
			g/[	 ]N[	 ]/s//	Y	/
			w
			q
		END
	   fi  
	done

	cd $ROOT/$LCL_MACH/etc/conf/mdevice.d/.save
	mv $* .. >/dev/null 2>&1
}

stub()
{
	cd $ROOT/$LCL_MACH/etc/conf/sdevice.d/.save
	mv $* .. >/dev/null 2>&1
	cd ..

	for i in `grep -l '[	 ]Y[	 ]' $* 2>/dev/null`
	do
		ed -s $i <<-END
			g/[	 ]Y[	 ]/s//	N	/
			w
			q
		END
	done
	cd $ROOT/$LCL_MACH/etc/conf/mdevice.d/.save
	mv $* .. >/dev/null 2>&1
}

make_static()
{
	cd $ROOT/$LCL_MACH/etc/conf/sdevice.d
	for i in $*
	do
		ed -s $i <<-END
			/\$version/a
			\$static
			.
			w
			q
		END
	done
}

tune()
{
	MACH=$LCL_MACH idtune -f PAGES_UNLOCK 50
	MACH=$LCL_MACH idtune -f FLCKREC 100
	MACH=$LCL_MACH idtune -f NPROC 100
	MACH=$LCL_MACH idtune -f MAXUP 30
	MACH=$LCL_MACH idtune -f NHBUF 32
	MACH=$LCL_MACH idtune -f FD_DOOR_SENSE 0
	MACH=$LCL_MACH idtune -f ARG_MAX 20480
}

make_space()
{
	cd $ROOT/$LCL_MACH/etc/conf/sdevice.d/.save
	for i in *
	do
		rm -rf $ROOT/$LCL_MACH/etc/conf/pack.d/$i
		rm -rf $ROOT/$LCL_MACH/etc/conf/mod.d/$i
		rm -rf $ROOT/$LCL_MACH/etc/conf/mtune.d/$i
		rm -rf $ROOT/$LCL_MACH/etc/conf/autotune.d/$i
	done
	cd ..
	rm -rf .save
	rm -rf $ROOT/$LCL_MACH/etc/conf/mdevice.d/.save
	rm -f $ROOT/$LCL_MACH/etc/conf/cf.d/stune*
}

mini_kernel()
{
	PATH=$ROOT/$LCL_MACH/etc/conf/bin:$PATH

	STATIC_LIST="ansi asyc atup ca ccnv cdfs char clone cmux confmgr cram 
	dcompat dma dosfs eisa elf fd fdbuf fifofs fpe fs gentty gvid iaf iasy 
	intmap intp io kd kdvm kernel kma ldterm mca mem memfs mm mod modksym 
	name namefs nullzero pci pit proc procfs pstart ramd resmgr rtc s5 sad 
	sc01 sd01 sdi specfs st01 sum svc sysclass ts udev util ws"

	if [ "$LANG" = "ja" ]
	then
		STATIC_LIST="$STATIC_LIST gsd fnt"
	fi

	DYNAMIC_LIST="$MOD_LIST"

	STUB_LIST="asyhp async audit coff dac event ipc log mac merge 
	nfs prf pse rand segdev sysdump tpath xnamfs xque kdb_util"

	turnoff
	turnon $STATIC_LIST $DYNAMIC_LIST
	stub $STUB_LIST
	make_static $STATIC_LIST
	tune
	make_space
}

create_boot_passwd ()
{
	echo "rootdev=ramd(0,0)" > $PROTO/boot
	echo "rootfs=s5" >> $PROTO/boot
	BOOTMSG=`pfmt -l UX:drf_437 -s NOSTD -g drf:27 "Booting from the Emergency Recovery Floppy..." 2>&1`
	echo "BOOTMSG=$BOOTMSG" >> $PROTO/boot

	sed -e "/^MIP=/d"        \
	    -e "/^SIP=/d"        \
	    -e "/^BOOTMSG=/d"    \
	    -e "/^AUTOBOOT=/d"    \
	    -e "/^KERNEL=/d"     \
	    -e "/^DISK=/d"       \
	    -e "/^SLOWBOOT=/d"   \
	    -e "/^RESMGR=/d"     \
	    -e "/^TIMEOUT=/d"    \
	    -e "/^rootfs=/d"     \
	    -e "/^rootdev=/d"  /stand/boot >> $PROTO/boot

	sed 's/ttcompat//g' /etc/ap/chan.ap > $PROTO/chan.ap.flop
	awk -F: ' NF > 0 { if ( $3 < 100 )
			print $0 } ' /etc/passwd > $PROTO/passwd
	awk -F: ' NF > 0 { if ( $3 < 100 )
			print $0 } ' /etc/group > $PROTO/group
}
conframdfs ()
{
	PATH=$PROTO/bin:$TOOLS/usr/ccs/bin:$PATH export PATH
	EDSYM="/usr/lib/drf/edsym"

	BASE=$ROOT/$LCL_MACH
	SOURCE_KERNEL=$BASE/stand/unix.$medium
	DEST_KERNEL=$BASE/stand/unix.drf

	[ ! -s $SOURCE_KERNEL ] && {
		echo "$SOURCE_KERNEL does not exist." >> ${DRF_LOG_FL}
		return 1
	}
	cp $SOURCE_KERNEL $DEST_KERNEL 

	MEMSZ=`memsize`
	sed -e "s/LANG=XXX/LANG=$LANG/" \
	    -e "s/LC_CTYPE=XXX/LC_CTYPE=$LANG/" \
	    -e "s/KEYBOARD=XXX/KEYBOARD=$KEYBOARD/" \
	    -e "s/MEMSZ=XXX/MEMSZ=$MEMSZ/" \
		/usr/lib/drf/drf_inst.gen \
		> $PROTO/drf_inst

	sizeline=""
	sizeline=`$NM -px $SOURCE_KERNEL | grep RootRamDiskBuffer`
	set $sizeline >/dev/null
	
	if [ "$3" != "RootRamDiskBuffer" ]
	then
		echo "Cannot find symbol RootRamDiskBuffer in file $SOURCE_KERNEL ." >> ${DRF_LOG_FL}
		return 1
	fi
	bufferaddr=$1

	LCL_TEMP=/tmp/ramd$$ export LCL_TEMP
	ROOTFS=$LCL_TEMP/ramdisk.fs
	mkdir $LCL_TEMP
	if [ -s /usr/lib/drf/locale/$LANG/txtstr ] 
	then
		DLANG=$LANG
	else
		DLANG=C
	fi

	#Uncomment all locale-specific lines
	#Delete all other comment lines
	sed \
		-e "s,^#$LANG,," \
		-e "s,^#.*,," \
		-e "s,\$LANG,$LANG," \
		-e "s,\$DLANG,$DLANG," \
		-e "s,\$PROTO,$PROTO," \
		-e "s,\$ROOT,$ROOT," \
		-e "s,\$MACH,$MACH," \
		-e "s,\$RESMGR,$RESMGR," \
		$RAMPROTO \
		> $LCL_TEMP/ramdproto
	> $ROOTFS

	create_boot_passwd

	echo "\nMaking file system image.\n" >> ${DRF_LOG_FL}
	/sbin/mkfs -Fs5 -b 512 $ROOTFS $LCL_TEMP/ramdproto 2 36 2>$LCL_TEMP/mkfs_log >> ${DRF_LOG_FL}
	if [ -s $LCL_TEMP/mkfs_log ]
	then
		echo  "mkfs of ramdisk filesystem failed" >> ${DRF_LOG_FL}
        	cat $LCL_TEMP/mkfs_log >> ${DRF_LOG_FL}
        	return 1
	fi

	rm $PROTO/chan.ap.flop $PROTO/passwd $PROTO/group
	unset FIRST_DATA
	$DUMP -hv $DEST_KERNEL | grep '\.data$' |
	while read x1 x2 x3 section offset x4 name
	do
		if [ -z "$FIRST_DATA" ]
		then
			FIRST_DATA="found"
			continue # skip the first data section
		else
			echo "\nWriting the file system image into the kernel .data section." >> ${DRF_LOG_FL}
			echo "Address of RootRamDiskBuffer is $bufferaddr" >> ${DRF_LOG_FL}
			$EDSYM -f $ROOTFS $section $offset $bufferaddr $DEST_KERNEL || 
			{
			  echo "Cannot find a .data section in $DEST_KERNEL ." >> ${DRF_LOG_FL}
			  return  1
			}
			break
		fi
	done
	echo "\nStripping symbol table from $DEST_KERNEL" >> ${DRF_LOG_FL}
	$STRIP $DEST_KERNEL
	echo "\nEmptying .comment section of $DEST_KERNEL" >> ${DRF_LOG_FL}
	$MCS -d  $DEST_KERNEL
	rm -rf $LCL_TEMP
	return 0
}

build_kernel()
{
	echo "\nCreating a temporary kernel build tree in $ROOT/$LCL_MACH" >> ${DRF_LOG_FL}
	rm -f $ROOT/$LCL_MACH/stand/unix.drf*	# Informs conframdfs that a new
												# unix is being built.
	mkdir $FLOP_TMP_DIR
	[ -d $ROOT/$LCL_MACH/stand ] || mkdir -p $ROOT/$LCL_MACH/stand
	[ -d $ROOT/$LCL_MACH/etc/conf/mod.d ] || mkdir -p $ROOT/$LCL_MACH/etc/conf/mod.d
	[ -d $ROOT/$LCL_MACH/etc/conf/modnew.d ] || mkdir -p $ROOT/$LCL_MACH/etc/conf/modnew.d
	(
	cd /
	find etc/conf/*.d etc/initprog -print |
		egrep -v 'unix$|\.o$|mod\.d|modnew\.d' |
		cpio -pdumV $ROOT/$LCL_MACH >/dev/null 2>&1
	find etc/conf/pack.d \( -name Driver.o -o -name Driver_atup.o -o -name Modstub.o \) -print |
		cpio -pdumV $ROOT/$LCL_MACH >/dev/null 2>&1
	chmod 0644 $ROOT/$LCL_MACH/etc/initprog/?ip $ROOT/$LCL_MACH/etc/initprog/dcmp
	)
	[ -h $ROOT/$LCL_MACH/etc/conf/bin ] ||
		ln -s /etc/conf/bin $ROOT/$LCL_MACH/etc/conf/bin

	echo "\nReconfiguring files under $ROOT/$LCL_MACH/etc/conf" >> ${DRF_LOG_FL}

	( mini_kernel )

	echo "\nBuilding MOD list" >> ${DRF_LOG_FL}
	(
	cd $ROOT/$LCL_MACH/etc/conf/sdevice.d
	for i in $MOD_LIST
	do
		grep '^\$static' $i >/dev/null 2>&1
		if [ $? -eq 0 ] 
		then
			ed -s $i > /dev/null <<-END
				/^\$static
				d
				w
				q
			END
		fi
	done

	MOPTS=`for i in $MOD_LIST
		do
			echo "-M $i \c"
		done`

	echo "\nDoing idbuild -M to create loadable modules." >> ${DRF_LOG_FL}
	echo "The loadable modules option : $MOPTS" >> ${DRF_LOG_FL}

	MACH=$LCL_MACH /etc/conf/bin/idtype atup

	IDBLD_ERRS=/tmp/drf_$$_moderr
	MACH=$LCL_MACH /etc/conf/bin/idbuild \
	     -I/usr/include ${MOPTS} -# > ${IDBLD_ERRS} 2>&1 || {
		  echo "The idbuild of ${MOPTS} failed with error = $? " >> ${DRF_LOG_FL}
		  echo "Check the ${IDBLD_ERRS} file for the idbuild failure reasons. " >> ${DRF_LOG_FL}
		  return 1
		}

	rm -f ${IDBLD_ERRS}
	mv $ROOT/$LCL_MACH/etc/conf/mod.d/* $ROOT/$LCL_MACH/etc/conf/modnew.d 2>/dev/null
	cd $ROOT/$LCL_MACH/etc/conf/modnew.d
	echo "\nExamining symbol tables of various loadable modules." >> ${DRF_LOG_FL}
	for i in $MOD_LIST
	do
		[ -f $i ] || {
			echo " Cannot find $ROOT/$LCL_MACH/etc/conf/modnew.d/$i" >> ${DRF_LOG_FL}
			return 1
		}
		$NM $i | grep UNDEF | sed -e 's/.*|//' > $FLOP_TMP_DIR/${i}list
	done
	) || return 1
	sed -e '/#/D' -e '/^$/D' /usr/lib/drf/staticlist  > $FLOP_TMP_DIR/staticlist
	(
	for i in $MOD_LIST static
	do
		cat $FLOP_TMP_DIR/${i}list
	done
	) | LC_ALL=C sort -u > $FLOP_TMP_DIR/symlist

	echo "\nModifying $ROOT/$LCL_MACH/etc/conf/pack.d/ramd/space.c" >> ${DRF_LOG_FL}
	t=`sed -n -e '3s/^\([0-9]*\) [0-9]*$/\1/p' ${RAMPROTO}`
	s=`sed -n -e '/^#define RAMD_SIZE	(/p' $ROOT/$LCL_MACH/etc/conf/pack.d/ramd/space.c`
	z=`expr "$s" : '^#define RAMD_SIZE	(\([0-9]*\).*'`
	w=`expr "$z" \* 2`
	t=`expr "$t" + 3`
	p=`expr "$t" / 2`
	ed -s $ROOT/$LCL_MACH/etc/conf/pack.d/ramd/space.c <<-EOF
	/RAMD_SIZE/s/$z/$p/
	w
	q
	EOF

	IDBLD_ERRS=/tmp/drf_$$_kernerr
	echo "\nBuilding the mini-kernel." >> ${DRF_LOG_FL}
	MACH=$LCL_MACH /etc/conf/bin/idbuild ${SYMS} \
		-DRAMD_BOOT -I/usr/include  -# \
		-O $ROOT/$LCL_MACH/stand/unix.$medium > ${IDBLD_ERRS} 2>&1 || {
		    echo "The mini-kernel build failed with error = $? " >> ${DRF_LOG_FL}
		    echo "Check the ${IDBLD_ERRS} file for the mini-kernel failure reasons. " >> ${DRF_LOG_FL}
		    return 1
		 }

	rm -f ${IDBLD_ERRS}
	[ -d $PROTO/stage ] || mkdir $PROTO/stage
	rm -rf $FLOP_TMP_DIR
	return 0
}

f_pstamp()
{
	if [ -w $1 ]
	then
		$STRIP $1 > /dev/null 2>&1
		$MCS -d $1 > /dev/null 2>&1
	else
		echo  " Cannot write file $1, not mcs'ing" >> ${DRF_LOG_FL}
	fi
}

strip_em()
{
	echo "\nStripping various files." >> ${DRF_LOG_FL}
	for name in mip sip dcmp
	do
		f_pstamp $ROOT/$LCL_MACH/etc/initprog/$name
	done
	return 0
}


image_make1()
{
	set -e

	COMPRESS=/usr/lib/drf/bzip

	${COMPRESS} -s28k ${ROOT}/${LCL_MACH}/etc/initprog/mip > mip

	${COMPRESS} -s28k ${ROOT}/${LCL_MACH}/etc/initprog/sip > sip 

	${COMPRESS} -s28k ${ROOT}/${LCL_MACH}/stand/${UNIX_FILE} > ${UNIX_FILE}

	[ -s ${ROOT}/${LCL_MACH}/etc/initprog/${LOGO_IMG} ] &&
	${COMPRESS} -s28k ${ROOT}/${LCL_MACH}/etc/initprog/${LOGO_IMG} > ${LOGO_IMG}
	cp ${ROOT}/${LCL_MACH}/etc/initprog/dcmp dcmp 

	${COMPRESS} -s28k /stand/${RESMGR} > resmgr

	set +e
	return 0
}

image_make2 ()
{
	rm -rf $PROTO/stage/.extra.d
	mkdir -p $PROTO/stage/.extra.d/etc/fs
	for i in $FSCKS
	do
	    mkdir $PROTO/stage/.extra.d/etc/fs/$i
	    ln -s /usr/lib/drf/fscks/$i.fsck $PROTO/stage/.extra.d/etc/fs/$i/fsck
	    ln -s /usr/lib/drf/fscks/$i.mkfs $PROTO/stage/.extra.d/etc/fs/$i/mkfs
	    [ "$i" = "bfs" ] && continue
	    ln -s /etc/fs/$i/labelit $PROTO/stage/.extra.d/etc/fs/$i/labelit
	done

	mkdir -p $PROTO/stage/.extra.d/etc/conf/mod.d
	for i in $FSMODS
	do
	    $STRIP  $ROOT/$LCL_MACH/etc/conf/modnew.d/$i
	    $MCS -d $ROOT/$LCL_MACH/etc/conf/modnew.d/$i
	    ln -s $ROOT/$LCL_MACH/etc/conf/modnew.d/$i $PROTO/stage/.extra.d/etc/conf/mod.d/$i
	done
	return 0
}

cut_drf()
{

	LOGO_IMG=logo.img export LOGO_IMG

	UNIX_FILE=unix.drf export UNIX_FILE

	cd $PROTO/stage
	echo "${FDRIVE}\t${BLKCYLS}\t${BLKS}" > $PROTO/stage/drive_info
	rm -f unix
	image_make1  || return $?
	ln $UNIX_FILE unix
	cp $PROTO/boot boot
	[ -s ${LOGO_IMG} ] || LOGO_IMG=

	/usr/lib/drf/sbfpack /etc/fboot dcmp mip sip boot $LOGO_IMG \
		unix resmgr /dev/rdsk/${FDRIVE}t  >> ${DRF_LOG_FL} || return $?

	pfmt -l UX:drf -s NOSTD -g drf:21 "\n\tRemove the floppy from the floppy drive. \n\tInsert the second floppy and Press <ENTER>."
	read ans

	#	Check whether diskette is formatted or not
	#
	teststr="The emregency_disk_test_string"
	testfile=/tmp/tst_drf.$$
	
	while echo "${teststr}" > ${testfile}
	do
	   if echo ${testfile} | cpio -oc -G STDIO -O /dev/dsk/${FDRIVE}t 2>/dev/null 1>&2
	   then
		:	# The disk is formatted
	   else
		echo
		pfmt -l UX:drf -s NOSTD -g drf:5 "	There is no floppy in the drive, or the floppy is\n	write-protected, or the floppy is not formatted.\n	Enter (f)ormat, (r)etry, or (q)uit: "
		read ans
		echo
		case "${ans:-q}" in
	  	   f) /usr/sbin/format -i1 /dev/rdsk/${FDRIVE}t > /dev/null 2>&1
	     	      continue
	     	      ;;

	  	   r) continue ;;
	
	  	   *) rm -f ${testfile}
		      NO_SECOND_FLP=1
	     	      return 0 ;;
		esac
	   fi
	   break
	done
	rm -f ${testfile}
	image_make2
	cd  $PROTO/stage
	find .extra.d -depth -print | cpio -oDL -H crc > cpioout 2>/dev/null
	/usr/lib/drf/bzip -s16k cpioout > cpioout.z
	/usr/lib/drf/wrt -d /dev/dsk/${FDRIVE}t cpioout.z
	ret_val=$?
	rm -f cpioout cpioout.z
	return $ret_val
}

#main()
medium=tape
PATH=:/usr/lib/drf:$PATH export PATH
RAMPROTO="$PROTO/drfram.proto" export RAMPROTO
cp /usr/lib/drf/drfram.proto $RAMPROTO
FLOP_TMP_DIR=/tmp/flop.$$ export FLOP_TMP_DIR
NM=/usr/ccs/bin/nm
DUMP=/usr/ccs/bin/dump
STRIP=/usr/ccs/bin/strip 
MCS=/usr/bin/mcs 
export NM DUMP STRIP MCS
export MOD_LIST _DRVMODS LCL_TEMP 
SYMS="-l $FLOP_TMP_DIR/symlist" export SYMS
eval `defadm locale LANG 2>/dev/null`
eval `defadm keyboard KEYBOARD 2>/dev/null` 
[ "$KEYBOARD" = "NONE" ] && KEYBOARD=""
LANG=${LANG:-C}  
export LANG KEYBOARD 

echo "\n\t The log messages from prep_flop " > ${DRF_LOG_FL}

if [ "$LANG" = "ja" ]
then
	add_terminfo_mb    # add multi-byte terminfo to ram proto file
elif [ "$LANG" = "C" -a -z "$KEYBOARD" ] ||
        [ "$KEYBOARD" = "AX" ] || [ "$KEYBOARD" = "A01" ]
then
	add_terminfo      # add AT386 terminfo entries to ram proto file
else
	add_terminfo_ie   # add AT386-ie terminfo entries ro ram proto file
fi
[ -z "$KEYBOARD" ] || add_keyboard   # add keyboard info to ram proto file
[ "$LANG" != "C" ] && add_lc_ctype   # Add the LC_CTYPE file to ram proto file
echo "exit 0" > $PROTO/putdev
echo "/dev/vt /dev/kd/kd	3" > $PROTO/work.1

RESMGR= export RESMGR
eval `grep "^RESMGR=" /stand/boot`
[ -z "$RESMGR" ] && RESMGR=resmgr
set_mod_list
NO_SECOND_FLP=0 export NO_SECOND_FLP
error=1
build_kernel && strip_em && conframdfs && cut_drf && error=0

[ "$error" = "0" -a "$NO_SECOND_FLP" = "1" ] && error=2
rm -f $PROTO/drfram.proto $PROTO/putdev $PROTO/work.1 $PROTO/drf_inst
echo "\nprep_flop: Done." >> ${DRF_LOG_FL}
rm -rf $LCL_TEMP $FLOP_TMP_DIR 

exit $error
