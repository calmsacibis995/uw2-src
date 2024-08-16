#ident	"@(#)filemgmt:i386/cmd/oamintf/files/make/Form.make	1.3"
#ident	"$Header: $"
Form="Create A File System (make)"
help=OPEN TEXT $INTFBASE/Text.itemhelp $LININFO
close=`rm /var/tmp/make.$VPID /tmp/make.out 2>/dev/null;
        unset -l XRET -l LABEL -l MNTPT -l FSTYPE -l DEVICE;
        unset -l RVAL -l FLOPPY -l EXIT`

framemsg=`readfile $INTFBASE/form.msg`

done=`indicator -w;
	if [ "$MKDIR" = "yes" ];
        then
                if [ ! -d "$F6" ];
                then
                        mkdir $F6 > /dev/null;
                fi;
        fi;
	rm /var/tmp/make.$VPID 2>/dev/null;
        grep "^[^#].*" /etc/device.tab | fgrep $F1 > /var/tmp/make.$VPID;
	if [  ! -b "$F1" ];
        then
                if [ -s /var/tmp/make.$VPID ];
                then
                        /usr/bin/devattr "$F1" bdevice 2> /dev/null | set -l DEVICE;
                else
                        set -l DEVICE=$F1;
                fi;
        else
                set -l DEVICE=$F1;
        fi;
	set -l  FSTYPE=$F2;
	if [ "$F3" ];
	then
		set -l LABEL=$F3;
	else
		set -l LABEL="NULL";
	fi;
	if [ "$F4" = "yes" ];
	then
		set -l MNTPT=$F6;
	else
		unset -l MNTPT;
	fi;
	if [ -n "$O_MNTPT" ] && [ -n "$MNTPT" ] && [ "$O_MNTPT" != "$MNTPT" ];
                then
                        set -l CMD="OPEN FORM $OBJ_DIR/Form.mount $O_MNTPT $MNTPT"; 
                else
                        ifdiskette $DEVICE | set -l FLOPPY;
                        if [ "$FLOPPY" = "false" ];
                        then
                                $OAMBASE/bin/call_fstyp $DEVICE | set -l XRET;        
                        else
                                set -l XRET="1";
			fi;
			if [ "$XRET" = "0" ];
                        then
                                set -l CMD="OPEN FORM $OBJ_DIR/Form.make2";
                        else
                                set -l CMD="OPEN FORM $OBJ_DIR/Form.$FSTYPE";
                        fi;
                fi;
        fi;
	`$CMD
	

name="Device that will contain the file system:"
lininfo=Form.make:F1
nrow=1
ncol=1
frow=1
fcol=43
rows=1
columns=20
rmenu=OPEN MENU $OBJ_DIR/../Menu.fsdevch
#rmenu={ `$OAMBASE/bin/dev cdevice` }
value=`if i386;
        then
                echo diskette1;
        else
                if [ -c "$ARG2" ];
                then
                        echo "$ARG2";
                fi;
      fi`
valid=`message -w; $OAMBASE/bin/validdev "$F1" `
invalidmsg="Error - $F1 is not a valid device name"

name="File system type:"
lininfo=Form.make:F2
nrow=2
ncol=1
frow=2
fcol=19
rows=1
columns=20
value=`if [ "$F1" ];
	then
		$OAMBASE/bin/fstyp_spcl "$F1";
	fi;`
rmenu=OPEN MENU $OBJ_DIR/Menu.mkfsch
valid=`if [ -f /etc/fs/$F2/mkfs ];
	then
		echo true;
	else
		echo false;
	fi`
invalidmsg="Press CHOICES to select valid response"

name="Label for the  file system:"
lininfo=Form.make2:F3
nrow=3
ncol=1
frow=3
fcol=29
rows=1
columns=6
valid=`regex -v "$F3" '^[0-9A-Za-z]{0,6}$'`
invalidmsg=const 'Must be 1 to 6 alphanumeric characters (e.g. fsys01)'
show=`regex -v "$F2" 'bfs' FALSE '^.*' TRUE`

name="Once created, should the new file system be mounted?"
lininfo=Form.make2:F2
nrow=4
ncol=1
frow=4
fcol=54
rows=1
columns=3
value=yes
rmenu={ yes no }
menuonly=true
invalidmsg="Press CHOICES to select valid response."

name="Create the mount point if not already created?"
lininfo=Form.make2:F4
nrow=5
ncol=1
frow=5
fcol=48
rows=1
columns=3
show=`[ "$F4" = "yes" ]`
value=yes
rmenu={ yes no }
menuonly=true
invalidmsg="Press CHOICES to select valid response."

name="File system name when mounted:"
lininfo=Form.mntpt:F1
nrow=6
ncol=1
frow=6
fcol=32
rows=1
columns=14
show=`[ "$F4" = "yes" ]`
value=`if [ "$F1" ];
        then $OAMBASE/bin/invfstab "$F1" | set -l RVAL2;
		if [ "$RVAL2" = "true" ];
                then 
                        /usr/bin/cut -d" " -f2  /tmp/invfstab | set -l O_MNTPT;
                        if [ "$O_MNTPT" = "-" ];
                        then 
                                set -l O_MNTPT="";
                        fi;
                        /bin/rm /tmp/invfstab;
                else      
                        set -l O_MNTPT="";
                fi;
                echo "$O_MNTPT";
        fi`
validOnDone=`if [ ! -d $F6 ];
	 then
                if [ "$F5" = "yes" ] ;
                then
                        set -l MKDIR="yes";
                        set -l XRET="0";
                else
                        set -l MKDIR="no";
                        set -l XRET="1";
                fi;
        else
                mount | grep "^$F6.*[   ]" > /dev/null 2>/dev/null;
                set -l XRET="$RET";
                if [ "$XRET" = "0" ];
                then set -l XRET="2";
                else set -l XRET="0";
                fi;
	fi;
        [ "$XRET" = "0" ]`
invalidOnDoneMsg=`regex -v "$XRET"
        '^1$' "Error -- $F6 not a valid mount point"
        '^2$' "Error -- $F6 currently in use as a mount point" `
