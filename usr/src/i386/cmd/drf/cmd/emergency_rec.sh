#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)drf:cmd/emergency_rec.sh	1.15"

#This function maps the OS partition type to the corresponding
#number used by fdisk command
find_os_num()
{
	OS_NUM=0
	case "$1" in

		"UNIX System" )
			OS_NUM=1
			;;
		"pre-5.0DOS" ) 
			OS_NUM=2
			;;
		"DOS" ) 
			OS_NUM=3
			;;
		"System" )
			OS_NUM=4
			;;
		"Other" )
			OS_NUM=5
			;;
		*)	;;
	esac
}

#This function creates the ${FD_CMDS1} file corresponding to $1
#FD_CMDS1 file will contain the commands to be executed to create the
#unix and system partition if present and if it is first disk

fdisk_cmds()
{
   RTDEV=`devattr $1 cdevice 2>/dev/null`

   FDSKOUT=/tmp/fd_out export FDSKOUT
	LC_ALL=C fdisk -L $RTDEV >${FDSKOUT} <<-END
x
	END
   RDEV=`echo $RTDEV | sed "s/..$//"`
   eval `grep NPART ${FDSKOUT}`
   echo "fdisk -L \$1 >/tmp/fdisk_out <<-END" > ${FD_CMDS1}

   > ${PART_DD_INF}
   > ${FD_CMDS2}
   PART=1
   # Check for Unix and system partition and write the create the command
   # to create it in to FD_CMDS1 file and the boundary info to FD_CMDS2. 
   # Update the PART_DD_INF file to contain the info about system partition 
   # to be used by dd to backup/restore the system partition. 
   while [ $PART -le $NPART ]
   do
	OS_LINE=`grep PART$PART ${FDSKOUT}`
	OS_STAT=`echo "$OS_LINE" | cut -f2`
	OS_TYPE=`echo "$OS_LINE" | cut -f3`
	BEG_CYL=`echo "$OS_LINE" | cut -f4`
	END_CYL=`echo "$OS_LINE" | cut -f5`
	SIZE_CYL=`echo "$OS_LINE" | cut -f6`
	find_os_num "$OS_TYPE"
	OS_ST=""
	if [ "$OS_STAT" = "Active" -a $OS_NUM -eq 1 ] 
	then
		OS_ST=1
		PART_CMD="c $OS_NUM $BEG_CYL $SIZE_CYL $OS_ST"
		echo $PART_CMD >>${FD_CMDS1}
		PART_CMD="$BEG_CYL $END_CYL"
		echo $PART_CMD >> ${FD_CMDS2}
	else
		#Make a note if it system partition on first disk
		[ $OS_NUM -eq 4 -a $2 -eq 1 ] && {
		        Dd_dev="${RDEV}p${PART}"	
		        echo "$Dd_dev  p$PART"  > ${PART_DD_INF}
			PART_CMD="c $OS_NUM $BEG_CYL $SIZE_CYL $OS_ST"
			echo $PART_CMD >>${FD_CMDS1}
			PART_CMD="$BEG_CYL $END_CYL"
			echo $PART_CMD >> ${FD_CMDS2}
		}
	fi
	PART=`expr $PART \+ 1`
   done
   echo s >>${FD_CMDS1}
   echo END >>${FD_CMDS1}

}  # End of fdisk_cmds


#this function creates the $LAYOUT_FL to match the existing
#file systems in ${RDEV}s0 unix slice
#This file will be used to setup the corresponding disk

dskset_cmds ()
{
    LC_ALL=C prtvtoc -f $PRT_FOUT ${RDEV}s0
    >${LAYOUT_FL}
    for i in  1 2 3 4 6 10 11 12 13 14 15
    do
	ii=$i
	ap=$i
	if [ $i -lt 10 ] 
	then
	    ii=" $i"
	else
	    case "$i" in
	    	"10" ) ap="a"
			;;
	    	"11" ) ap="b"
			;;
	    	"12" ) ap="c"
			;;
	    	"13" ) ap="d"
			;;
	    	"14" ) ap="e"
			;;
	    	"15" ) ap="f"
			;;
	     	*) 
			;;
            esac
	fi

	grep "^$ii	0x0	0x0	0	0" < $PRT_FOUT > /dev/null
	[ $? -eq  0 ] && continue
	AN=`basename $RDEV`
 	BDSK=/dev/dsk/${AN}s${ap}
	case $i in
 	   "1")   MNTPNT="/"
		  ;;
	   "2")   MNTPNT="/dev/swap"
		  ;;
	   "6")   MNTPNT="/dev/dump"
		  ;;
	   "15")  MNTPNT="/dev/volprivate"
		  ;;
	   *)     
		MNTPNT=`grep "^$BDSK" /etc/vfstab | cut -f3`
		;;
	esac 
	
	if [ $i -eq 2 -o $i -eq 6 -o $i -eq 15 ] 
	then
		FStyp="-"
		BUFSZ="-"
	else
		if [ $i -eq 1 ] 
		then
		     FStyp=`grep "/dev/root" /etc/vfstab | cut -f4`
		else
		     FStyp=`grep "^$BDSK" /etc/vfstab | cut -f4`
		fi
		[ -z "$FStyp" ] && continue
        	Binfo=`mkfs -m -F $FStyp  $BDSK 2>/dev/null | grep bsize 2>/dev/null`
		if [ $? -eq 0 ]
		then
        	    BUFSZ=`echo "$Binfo" | sed 's/.*bsize=//' | cut -f1 -d','`
		else
		    case "$FStyp" in  #set the default value for the Fstyp
		       "bfs"  )		BUFSZ=512;;
		       "s5" | "vxfs" )	BUFSZ=1024;;
		       *) 		BUFSZ=4096;;
		    esac
		fi
	fi
		
	FLs=`grep "^$ii	0x" < $PRT_FOUT | cut -f5`
	[ -z "$FLs" ] && continue
	UN_TYP=M
	FLsiz=`expr $FLs \/ 2048`
	[ $FLsiz -eq 0 ] && {
		FLsiz=`expr $FLs \/ 2`
		UN_TYP=K
	}

	echo "$i	$MNTPNT	$FStyp	$BUFSZ	${FLsiz}${UN_TYP}	1R" >> ${LAYOUT_FL}
    done
}

#Set the variables for different output files for disk 1 and invoke fdisk_cmds
#and dskset_cmds function
disk1_setup ()
{
	rm -rf $WRK_DIR
	mkdir -p $WRK_DIR
	LAYOUT_FL=$WRK_DIR/lay_out_1 export LAYOUT_FL
	PRT_FOUT=$WRK_DIR/prt_f_1 export PRT_FOUT
	PART_DD_INF=$WRK_DIR/part_dd_1 export PART_DD_INF
	FD_CMDS1=$WRK_DIR/fd_cmds_11 export FD_CMDS1
	FD_CMDS2=$WRK_DIR/fd_cmds_12 export FD_CMDS2
	fdisk_cmds disk1 1
	dskset_cmds
	rm -f $PRT_FOUT
}

#Set the variables for different output files for disk 2 and invoke fdisk_cmds
#and dskset_cmds function
disk2_setup ()
{
	LAYOUT_FL=$WRK_DIR/lay_out_2 export LAYOUT_FL
	PRT_FOUT=$WRK_DIR/prt_f_2 export PRT_FOUT
	PART_DD_INF=$WRK_DIR/part_dd_2 export PART_DD_INF
	FD_CMDS1=$WRK_DIR/fd_cmds_21 export FD_CMDS1
	FD_CMDS2=$WRK_DIR/fd_cmds_22 export FD_CMDS2
	fdisk_cmds disk2 2
	dskset_cmds
	rm -f $PRT_FOUT $PART_DD_INF 
}

check_disk2 ()
{
	devattr disk2 1>/dev/null 2>&1
	[ $? -ne 0 ] && return
	RT2=`devattr disk2 addcmd | cut -f 2 -d" "`
	for i in /usr /home /home2
	do
	   spl=`grep "[	]$i[	]" /etc/vfstab | cut -f1`
	   if [ $? -eq 0 ] 
	   then
		echo $spl | grep $RT2 >/dev/null 2>&1
		[ $? -eq 0 ] && {
		    disk2_setup
		    return
	  	}
	   fi
	done
	return
}

dd_copy_disk()
{
	pfmt -l UX:drf -s NOSTD -g drf:26 "\n\tCopying the primary disk to tape, please wait ...\n"
	cd /tmp
	rm -rf $WRK_DIR
	mkdir -p $WRK_DIR
	tapecntl -t $TAPEDEV
	tapecntl -w $TAPEDEV
	tapecntl -f 512 $TAPEDEV
	OUTDEV=`devattr $MEDIUM norewind`
	DV=`devattr disk1 addcmd | cut -f2 -d" "`
	echo "Entire Disk" > $WRK_DIR/entire_dsk
	find $CPIO_DIR -depth -print | cpio -ocdu -O $OUTDEV 2>/dev/null 1>&2
	
	INDEV=/dev/rdsk/${DV}p0
	dd if=$INDEV of=$OUTDEV bs=64k 2>&1 >/dev/null
	if [ $? -eq 0 ]
	then
	    pfmt -l UX:drf -s NOSTD -g drf:12 "\n\tCreation of the Emergency Recovery tape was successful.\n\n"
	else
	    pfmt -l UX:drf -s NOSTD -g drf:25 "\n\tCreation of the Emergency Recovery tape was NOT successful.\n\n"
	fi
	return 
}

cpio_copy_disk()
{
	pfmt -l UX:drf -s NOSTD -g drf:11 "\n\tCopying the hard disk(s) to tape, please wait ...\n"
	cd /tmp
	tapecntl -t $TAPEDEV
	tapecntl -w $TAPEDEV
	tapecntl -f 512 $TAPEDEV
	OUTDEV=`devattr $MEDIUM norewind`
	cp /etc/boot $WRK_DIR
	cp /usr/sbin/disksetup $WRK_DIR
	find $CPIO_DIR -depth -print | cpio -ocdu -O $OUTDEV 2>/dev/null 1>&2
	[ -s $WRK_DIR/part_dd_1 ] && {
		read dd_to_cp dd_size < $WRK_DIR/part_dd_1
		dd if=$dd_to_cp of=$OUTDEV bs=512 >/dev/null 2>&1
	}
	CONT_MSG=`pfmt -l UX:drf -s NOSTD -g drf:23 "\n\tRemove the tape from the tape drive.\n\tInsert the next tape and press <ENTER>." 2>&1`
	cd /
	find . -depth -print | cpio -ocdu -M "$CONT_MSG" -O $OUTDEV 2>/dev/null 1>&2
	if [ $? -eq 0 -o $? -eq 2 ]
	then
	    pfmt -l UX:drf -s NOSTD -g drf:12 "\n\tCreation of the Emergency Recovery tape was successful.\n\n"
	else
	    pfmt -l UX:drf -s NOSTD -g drf:25 "\n\tCreation of the Emergency Recovery tape was NOT successful.\n\n"
	fi
	return 
 }

# Display message and exit
cleanup()
{
	trap '' 1 2 15

	echo
	pfmt -l UX:drf -s error -g drf:13 "Emergency Recovery tape creation aborted.\n\n"
	exit 1
}

Usage()
{
   echo
   pfmt -l UX:drf -s error -g drf:14 "Usage: emergency_rec [-e] ctape1|ctape2\n\n"
   exit 1
}


# Make sure that the system is in maintenance mode.
check_run_level()
{
   set `LC_ALL=C who -r`
   if [ "$3" != "S" -a "$3" != "s" -a "$3" != "1" ] 
   then
	echo
	pfmt -l UX:drf -s error -g drf:15 " You must be in maintenance mode to create the Emergency\n\t\tRecovery tape.\n\n"
	exit 1
   fi
}

mount_usrfs()
{
	[ -f /usr/lib/libc.so.1 ] && return 
	mount /usr  >/dev/null 2>&1
	[ $? -eq 0 ] && return
	while read spl fskdev mountp fstyp fsckpass atomnt mntpts macceil
	do
	    [ "$mountp" != "/usr" ] && continue
	    fsck -F $fstyp  -y $fskdev  >/dev/null 2>&1
	    mount -F $fstyp $spl $mountp >/dev/null 2>71
	    [ $? -eq 0 ] && return
	    pfmt -l UX:drf -s error -g drf:24 " Not able to mount /usr file systems.\n\t\tAborting the emergency tape creation.\n\n"
	    exit 1
	done < /etc/vfstab
	pfmt -l UX:drf -s error -g drf:24 " Not able to mount /usr file systems.\n\t\tAborting the emergency tape creation.\n\n"
	exit 1
}

mount_flsystms ()
{

    RT1=`devattr disk1 addcmd | cut -f2 -d" "`

    devattr disk2 1>/dev/null 2>&1 && 
	RT2=`devattr disk2 addcmd | cut -f2 -d" "`

    Not_mnted=""
    echo 0   > /tmp/$$_drf_abc
    while read special fsckdev mountp fstyp fsckpass automnt mountpts macceil
    do
	#skip the comment line
	fst_chr=`echo $special | cut -c1`
	[ "$fst_chr" = "#" ] && continue
	#skip if it / or /stand or /proc or /dev/fd
	[ "$mountp" = "/" -o "$mountp" = "/stand" -o "$mountp" = "/proc" -o "$mountp" = "/dev/fd" -o "$mountp" = "/usr" ] && continue
	#check if it is already mounted, this should not happen
	grep "[	]$mountp[	]" /etc/mnttab >/dev/null 2>&1
	[ $? -eq 0 ] && continue
	echo "$special" | grep $RT1 >/dev/null 2>&1
	if [ $? -eq 0 ] 
	then
	    mount -F$fstyp $special $mountp >/dev/null 2>&1
	    if [ $? -ne 0 ]
	    then
		fsck -F$fstyp -y $fsckdev >/dev/null 2>&1
		mount -F $fstyp $special $mountp >/dev/null 2>&1
		[ $? -ne 0 ] && {
			Not_mnted="$Not_mnted $mountp"
    			echo 1 $Not_mnted > /tmp/$$_drf_abc
		}
	    fi
	else
	    [ "$mountp" = "/home" -o "$mountp" = "/home2" ] || continue
	    [ -z "$RT2" ] && continue
	    echo "$special" | grep $RT2 >/dev/null 2>&1
	    if [ $? -eq 0 ]
	    then
		mount -F $fstyp $special $mountp >/dev/null 2>&1
            	if [ $? -ne 0 ]
            	then
                	fsck -F$fstyp -y $fsckdev >/dev/null 2>&1
                	mount -F $fstyp $special $mountp >/dev/null 2>&1
                	[ $? -ne 0 ] && {
				Not_mnted="$Not_mnted $mountp"
    				echo 1 $Not_mnted > /tmp/$$_drf_abc
			}
	    	fi
	    fi
	fi
    done < /etc/vfstab
    read err Not_mnted < /tmp/$$_drf_abc
    rm -f /tmp/$$_drf_abc
    if [ $err -eq 1 ]
    then
	pfmt -l UX:drf -s error -g drf:16 " Not able to mount %s file systems.\n\t\tAborting the emergency tape creation.\n\n" "$Not_mnted"
	exit 1
    fi
}

tape_ready ()
{
	while pfmt -l UX:drf -s NOSTD -g drf:17 "\n\tPlace a tape in %s and press <ENTER> or enter [q/Q] to abort : " $MEDIUM
	do
		read inp
		if [ "$inp" = "Q" -o "$inp" = "q" ] 
		then
			pfmt -l UX:drf -s NOSTD -g drf:18 "\n\tEmergency Recovery tape creation terminated.\n\n"
			exit 1
		fi
		ls /sbin/emergency_rec | cpio -ocdu -G STDIO -O $TAPEDEV 2>/dev/null
		[ $? = 0 ] && break
		echo
		pfmt -l UX:drf -s error -g drf:19 " Not able to write into the tape. Check the tape for damage\n\t\tor may be it is write protected.\n"
	done
}


#main

trap 'cleanup 1' 1 2 15

mount_usrfs

ENTire=NO
while getopts 'e\?' c
do
	case $c in
		e)	ENTire=YES
			;;
		\?)	Usage
			;;
		*)	Usage
			;;
	esac
done

shift `expr $OPTIND - 1`

[ $# -eq 1 ] || Usage

MEDIUM=$1
devattr ${MEDIUM} 1>/dev/null 2>&1 ||
	{ echo
	  pfmt -l UX:drf -s error -g drf:20 "Device %s is not present in /etc/device.tab.\n" $MEDIUM;
	  echo
	  exit 1;
	}

check_run_level

TAPEDEV=`devattr $MEDIUM cdevice`
WRK_DIR=/tmp/.extra.d/Drf_Rec export WRK_DIR  #Need to have some fixed name
CPIO_DIR=.extra.d/Drf_Rec export CPIO_DIR  #Need to have some fixed name

tape_ready

if [ "$ENTire" = "YES" ]
then
	dd_copy_disk
else
	disk1_setup

	mount_flsystms

	check_disk2

	cpio_copy_disk
fi

exit 0

