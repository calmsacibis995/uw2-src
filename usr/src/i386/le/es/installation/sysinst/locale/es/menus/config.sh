#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)es_le:i386/le/es/installation/sysinst/locale/es/menus/config.sh	1.12"
# For use in 'asknodename' script

# For use in 'asktime' script
max TIME_MSG_LEN CONF_TIME_MSG SYSTEM_TIME_MSG
max TIME_COL1_LEN Year Month Day Hour Minute Timezone
max TIME_WIN_LEN $MAXTZ_NAME+$TIME_COL1_LEN+3 CHANGE_TIME

# For use in 'choose' script

# For use in 'fd' script
echo FD_WIDTH[1]=3
echo FD_TITLE[1]="\"   \""
max FD_WIDTH[2] UNIX DOS PRE5DOS other unused Type
center -C Type FD_WIDTH[2]
echo FD_TITLE[2]="\"$Type\""
max FD_WIDTH[3] Boot NonBoot Status
center -C Status FD_WIDTH[3]
echo FD_TITLE[3]="\"$Status\""
max FD_WIDTH[4] Start 6
center -C Start FD_WIDTH[4]
echo FD_TITLE[4]="\"$Start\""
max FD_WIDTH[5] End 6
center -C End FD_WIDTH[5]
echo FD_TITLE[5]="\"$End\""
max FD_WIDTH[6] Percentage 4
center -C Percentage FD_WIDTH[6]
echo FD_TITLE[6]="\"$Percentage\""
max FD_WIDTH[7] Cylinders 4
center -C Cylinders FD_WIDTH[7]
echo FD_TITLE[7]="\"$Cylinders\""
max FD_WIDTH[8] MB 4
center -C MB FD_WIDTH[8]
echo FD_TITLE[8]="\"$MB\""

# For use in 'fd_fs' script
max FDFS_WIDTH FSCONF_ENTRY FDISK_1_ENTRY FDISK_2_ENTRY DISKCHK_ENTRY MENU_EXIT

# For use in 'diskcheck' script
max DISKCHK_WIDTH[1] DCHK_1 DCHK_2 DO_BOOTSECTOR
center -C DiskCheck DISKCHK_WIDTH[1]
echo DISKCHK_TITLE[1]="\"$DiskCheck\""
max DISKCHK_WIDTH[2] YN 4
echo DISKCHK_TITLE[2]="\"$YN\""

# For use in 'fs' script
# The path names are fixed and the max is 9
max FS_WIDTH[1] FileSystem 16
center -C Filesystem FS_WIDTH[1]
echo FS_TITLE[1]="\"$FileSystem\""
max FS_WIDTH[2] Description standDESC swapDESC rootDESC usrDESC homeDESC dumpDESC varDESC home2DESC tmpDESC
center -C Description FS_WIDTH[2]
echo FS_TITLE[2]="\"$Description\""

echo FSX_TITLE[1]="\"$Description\""
echo FSX_TITLE[2]="\"$Attribute\""
max FSX_WIDTH[1] fstypeDESC blocksDESC inodesDESC
max FSX_WIDTH[2] FsType off Attribute
center -C Description FSX_WIDTH[1]
center -C Attribute FSX_WIDTH[2]

# The file system names are not localized and their max is 5
max FS_WIDTH[3] FsType off 5
center -C FsType FS_WIDTH[3]
echo FS_TITLE[3]="\"$FsType\""

max FS_WIDTH[4] Size 4
center -C Size FS_WIDTH[4]
echo FS_TITLE[4]="\"$Size\""

echo FS_WIDTH[5]=${#Disknum}
echo FS_TITLE[5]="\"$Disknum\""

# For use in 'main' script
max MAINWIDTH SAVE_EXIT PKGENTRY HARD_DISK_ENTRY CHANGE_NAME CHANGE_TIME INTL_ENTRY CANCEL MAIN_TITLE

#For use in 'loadhba.sh' script
OIFS="$IFS"
IFS="$nl"
set -A a -- ${HBA_REINSERT}
max -s HBA_REINSERTCols ${a[@]}
let i=$(echo "$HBA_REINSERT" | wc -l)
echo HBA_REINSERTLines=$i
IFS="$OIFS"
unset OIFS
OIFS="$IFS"
IFS="$nl"

#For use in 'floppy2' script

OIFS="$IFS"
IFS="$nl"
set -A a -- ${Floppy2Wait}
max -s Floppy2WaitCols ${a[@]}
let i=$(echo "$Floppy2Wait" | wc -l)
echo Floppy2WaitLines=$i
IFS="$OIFS"
unset OIFS

OIFS="$IFS"
IFS="$nl"
set -A a -- ${IHVwait}
max -s IHVwaitCols ${a[@]}
let i=$(echo "$IHVwait" | wc -l)
echo IHVwaitLines=$i
IFS="$OIFS"
unset OIFS

# inetinst
max INETINSTWIDTH INETINST_CHOICE INETINST_TAPE INETINST_CDROM INETINST_DISKETTE INETINST_TCP INETINST_SPX CANCEL3

#For use in 'dcuprompt' script
unset OIFS
OIFS="$IFS"
IFS="$nl"
set -A a -- ${DCUprompt}
max -s dcupromptCols ${a[@]}
let i=$(echo "$DCUprompt" | wc -l)
echo dcupromptLines=$i
IFS="$OIFS"
unset OIFS

OIFS="$IFS"
IFS="$nl"
set -A a -- ${HBAwait}
max -s HBAwaitCols ${a[@]}
let i=$(echo "$HBAwait" | wc -l)
echo HBAwaitLines=$i
IFS="$OIFS"
unset OIFS

max KEY_WIDTH SERIAL_PROMPT2 KEY_PROMPT
