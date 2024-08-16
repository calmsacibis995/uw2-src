#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

:
#      @(#)pppconf.sh	1.2 STREAMWare for Unixware 2.0  source
:
#
# Copyrighted as an unpublished work.
# (c) Copyright 1987-1994 Lachman Technology, Inc.
# All rights reserved.
#
#      SCCS IDENTIFICATION
#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.
#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#


:
#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.pppd/pppconf.sh	1.5"
#ident	"$Header: $"

#
#	STREAMware TCP
#	Copyright 1987, 1993 Lachman Technology, Inc.
#	All Rights Reserved.
#

OK=0
FAIL=1

PATH=/bin:/usr/bin:/usr/sbin/:/etc:/etc/conf/bin

PPPHOSTS=/etc/inet/ppphosts
PPPAUTH=/etc/inet/pppauth
HOSTS=/etc/inet/hosts
INTERFACE=/etc/confnet.d/inet/interface
PPPSHELL=/usr/lib/ppp/ppp
PPPDIR=/usr/lib/ppp
PASSWORD=/etc/passwd

#######################################################
# info table for ppphosts options
#
IDLE="idle:forever:PPP connection inactivity timeout in minutes"
TOUT="tmout:3:Timeout per PPP protocol request in seconds"
CONF="conf:10:Maximum number of PPP configure requests"
TEM="term:2:Maximum number of PPP termination requests"
NAK="nak:10:Maximum allowable number of remote PPP configure requests"
MRU="mru:296:Maximum receive unit size in bytes"
ACCM="accm:00000000:Async control character map in hex"
PAPTMOUT="paptmout:1:PPP peer authentication timeout in minutes"
DEBUG="debug:0:PPP link debugging level"

PAP="pap:off:PPP password authentication"
MGC="nomgc:on:Magic number negotiation"
PROTCOMP="protcomp:off:LCP protocol field compression"
ACCOMP="accomp:off:HDLC address-control field compression"
IPADDR="ipaddr:off:IP address negotiation"
IP1172="rfc1172addr:off:Old IP address negotiation (RFC1172)"
VJ="VJ:off:Van Jacobson TCP header compression"
OLD="old:off:Compatibility with old PPP"


##############################################################################
#
# Remove temp files and exit with the status passed as argument
#
cleanup() {
	trap '' 1 2 3 15
	[ "$tmp" ] && rm -f $tmp*
	exit $1 
}


##############################################################################
#
# Print an error message
# Usage: Error "message"
# Argument is a quoted error message
#
Error() {
	echo "\nError: $*" >&2
	return 1
}


##############################################################################
#
# Print a warning message
# Usage: warning "message"
# Argument is a quoted warning message
#
warning() {
	echo "\nWarning: $*"
}



##############################################################################
#
# clear the screen if it is supported
#
clearscr() {
	# check if the terminal environment is set up
	[ "$TERM" ] && clear 2> /dev/null
}


##############################################################################
#
# Print a message
# Argument is a quoted message
#
message() {
	echo "$*"
}


##############################################################################
#
# usage: safemv file1 file2
# move file1 to file2 without chance of interruption
#
safemv() {
        trap "" 1 2 3 15
        mv $1 $2
        trap "cleanup $FAIL" 1 2 3 15
}


##############################################################################
#
# Do the actual work of putting an entry in /etc/inet/hosts. If doing this
# causes conflicts, then go ahead and add entry, but warn the user.
# Usage: put_hosts <IP addr> <hostname> <fully qual. name>
#
put_hosts() {
	_IP=$1
	_HOST=$2


	if grep -i "^$_IP[      ][      ]*$_HOST$" $HOSTS > /dev/null 2>&1 \
	|| grep -i "^$_IP[      ][      ]*$_HOST[       ]" \
               $HOSTS > /dev/null 2>&1
	then
       	 : done
	else
        	if grep -i "^$_IP" $HOSTS > /dev/null 2>&1 \
         	|| grep -i "[   ]$_HOST[        ]" $HOSTS > /dev/null 2>&1 \
        	 || grep -i "[   ]$_HOST$" $HOSTS > /dev/null 2>&1
         	then
               	  	warn_hosts "    $_IP    $_HOST"
          	fi
          		echo "$_IP\t$_HOST" >> $HOSTS
          		message "Adding host $_HOST to $HOSTS"
	fi
    	_IP="" ; _HOST=""
}

##############################################################################
#
# Print a warning message to the screen telling the user that there might
# be a conflict between the chosen /etc/inet/hosts values and an already 
# existing entry in /etc/inet/hosts.
#
warn_hosts() {
        message "WARNING: The selected values:"
        message "$1"
        message "may conflict with other entries in $HOSTS."
        message "Edit $HOSTS to resolve conflicts."
        sleep 2
}

#******************************************************
getyn() {
	while echo "$* (y/n) \c">&2
	do read yn rest 
		case $yn in
		[yY]) return 0 ;;
		[nN]) return 1 ;;
		*) echo "Answer y or n" ;;
		esac
	done
}

#******************************************************
sub_menu() {
	submenu_prompt="\n1) $1\n2) $2\nq) Quit\n\nEnter choice: \c"
	echo "\n$3"
	while echo "$submenu_prompt"
	do read cmd
		case $cmd in
		1) return 1 ;;
		2) return 2 ;;
		q) return 0 ;;
		*) echo "illegal choice" ;;
		esac
	done
}

#******************************************************
ask_ip() {

	IP_ADDR=

	IP_PROMPT="\nEnter $1: \c"
	if [ "$2" ] ; then
		IP_PROMPT="\nEnter $1 [$2]: \c"
	else
		IP_PROMPT="\nEnter $1: \c"
	fi
	while echo $IP_PROMPT
	do	read ip_addr
		case "$ip_addr" in
		[0-9]*.[0-9]*.[0-9]*.[0-9]*) IP_ADDR=$ip_addr
			break
			;;
		"")	if [ "$2" ] ; then
				IP_ADDR=$2
				break
			else
				echo "$1 must have n.n.n.n form"
			fi
			;;	
		-)	IP_ADDR=
			break
			;;
		*)	echo "$1 must have n.n.n.n form"
			;;
		esac
	done
}
		
#******************************************************
ask_mask() {

	NET_MASK=

	if [ "$2" ] ; then
		MASK_PROMPT="\nEnter $1 (\"-\" for default) [$2]: \c"
	else
		MASK_PROMPT="\nEnter $1 [default]: \c"
	fi
	while echo $MASK_PROMPT
	do	read net_mask
		case "$net_mask" in
		[0-9]*.[0-9]*.[0-9]*.[0-9]*) NET_MASK=$net_mask
			break
			;;
		"")	if [ "$2" ] ; then
				NET_MASK=$2
				break
			else
				NET_MASK=
				break
			fi
			;;	
		-)	NET_MASK=
			break
			;;	
		*)	echo "$1 must have n.n.n.n form"
			;;
		esac
	done
}

#******************************************************
ask_uucpname() {
 
	NAME=

	message "\nThe UUCP name is used by PPP to dial into the remote system.\nIf a UUCP name is not specified, outgoing calls will be disabled."
 
	if [ "$2" ] ; then
		NAME_PROMPT="\nEnter $1 (\"-\" for null) [$2]: \c"
	else
		NAME_PROMPT="\nEnter $1 (\"-\" for null): \c"
	fi
	while echo $NAME_PROMPT
	do	read name
		case "$name" in
		[a-zA-Z]*) NAME=$name
			break
			;;
		"")	if [ "$2" ] ; then
				NAME=$2
				break
			else
				echo "$1 must start with letter"
			fi
			;;
		-)	NAME=
			break;
			;;
		*)	echo "$1 must start with letter"
			;;
		esac
	done
}

#******************************************************
ask_name() {
 
	NAME=

	if [ "$2" ] ; then
		NAME_PROMPT="\nEnter $1[$2]: \c"
	else
		NAME_PROMPT="\nEnter $1: \c"
	fi
	while echo $NAME_PROMPT
	do	read name
		case "$name" in
		[a-zA-Z]*) NAME=$name
			break
			;;
		"")	if [ "$2" ] ; then
				NAME="$2"
				break
			else
				echo "$1 must start with letter"
			fi
			;;
		*)	echo "$1 must start with letter"
			;;
		esac
	done
}

#######################################################
add_user() {

	getyn "\nDo you want to create PPP login account $1?"
	case $? in
	      0) useradd -d $PPPDIR -s $PPPSHELL $1
		if [ "$?" != "0" ] ; then
			message "Add PPP login $NAME fail."
		else
			message "Enter a password for login \"$1\""
			passwd $1
			if [ "$?" != "0" ] ; then
				message "Change $1 password fail."
			fi
		fi
		;;
	     1) ;;
	esac 
}

#######################################################
delete_user() {

	getyn "Do you want to delete PPP login account $1?"
	case $? in
	      0) userdel $1
		 if [ "$?" != "0" ] ; then
			message "Delete PPP login $1 fail"
		 fi
		;;
	     1) ;;
	esac 
}
#######################################################
add_local() {
	
	# if there is no PPPAUTH file, creat it
	if [ ! -r "$PPPAUTH" ] ; then
		if [ -r /etc/inet/pppauth.samp ] ; then
			/usr/bin/cp /etc/inet/pppauth.samp $PPPAUTH
		else
			cat "#pppauth file" > $PPPAUTH
		fi
		/usr/bin/chown root $PPPAUTH
		/usr/bin/chgrp root $PPPAUTH
		/usr/bin/chmod 600 $PPPAUTH
	fi

	message "\nLocal host uses local host ID/password to identify itself." 
	ask_name "local host ID"
	LID="$NAME"
	ask_name "local host password"
	LPWD="$NAME"

	grep '^*' $PPPAUTH > /dev/null 2>&1
	if [ $? -eq 0 ] ; then
		# Get rid of old entry
		sed "/^*/d" $PPPAUTH > /tmp/pppauth$$
		safemv /tmp/pppauth$$ $PPPAUTH	
	fi
	echo "*$LID $LPWD" >> $PPPAUTH
}

#######################################################
add_remote() {

	# if there is no PPPAUTH file, creat it
	if [ ! -r "$PPPAUTH" ] ; then
		if [ -r /etc/inet/pppauth.samp ] ; then
			/usr/bin/cp /etc/inet/pppauth.samp $PPPAUTH
		else
			cat "#pppauth file" > $PPPAUTH
		fi
		/usr/bin/chown root $PPPAUTH
		/usr/bin/chgrp root $PPPAUTH
		/usr/bin/chmod 600 $PPPAUTH
	fi

	message "\nLocal host uses remote host's ID/password to authenticate remote host." 
	ask_name "remote host ID"
	RID="$NAME"
	ask_name "remote host password"
	RPWD="$NAME"

	entry=`grep "^\$RID[	 ]" $PPPAUTH`
	if [ -n "$entry" ] ; then
		# Get rid of old entry
		sed "/$RID/d" $PPPAUTH > /tmp/pppauth$$
		safemv /tmp/pppauth$$ $PPPAUTH	
	fi
	echo "$RID $RPWD" >> $PPPAUTH
}

#######################################################
rm_local() {

	if [ ! -r "$PPPAUTH" ] ; then
		message "\n$PPPAUTH file does not exist."
		return $OK
	fi

	entry=`grep "^\*" $PPPAUTH | sed 's/*//'` 
	if [ "$entry" ] ; then
		LID=`echo $entry | awk '{ print $1 }' `
		LPWD=`echo $entry | awk '{ print $2 }' `
		message "\nLocal host ID/password are: $LID/$LPWD"
	else
		message "\nLocal host has no ID/password"
		return $OK
	fi

	getyn "Are you sure you want to remove local host ID/password?"	
	case $? in
		0) sed "/^\*$LID/d" $PPPAUTH > /tmp/pppauth$$
		   safemv /tmp/pppauth$$ $PPPAUTH
		    ;;
		1) ;;
	esac 
}

#######################################################
rm_remote() {

	if [ ! -r "$PPPAUTH" ] ; then
		message "\n$PPPAUTH file does not exist."
		return $OK
	fi

	entry=`grep -v "^#" $PPPAUTH | grep -v "^*"` 
	if [ "$entry" ] ; then
		message "\nThe remote host IDs/passwords are:"
	else
		message "\nThere are no ID/password for remote hosts"
		return $OK
	fi

	entry=`grep -v "^#" $PPPAUTH | grep -v "^*" | sed 's/	/:/'|sed 's/ /:/'`
	for i in $entry
	do
		RID=`echo $i | awk -F: '{ print $1 }'` 
		RPWD=`echo $i | awk -F: '{ print $2 }'` 
		echo "$RID/$RPWD"
	done

	ask_name "host ID to be removed"
	RID="$NAME"

	entry=`grep "^$RID" $PPPAUTH ` 
	if [ "$entry" ] ; then
		getyn "Are you sure you want to remove ID/password for remote host $RID?"	
		case $? in
			0) sed "/^$RID/d" $PPPAUTH > /tmp/pppauth$$
			   safemv /tmp/pppauth$$ $PPPAUTH
			    ;;
			1) ;;
		esac 
	else
		message "There is no remote host named $RID"
		return $OK
	fi

}

##############################################################################
conf_interface() {

	PPP_HOST_IP=
	PPP_DEST_IP=
	PPP_DEST_NAME=
	PPP_NM=
	
	ask_ip "IP address for local PPP end"
	PPP_HOST_IP="$IP_ADDR"
	ask_ip "IP address for remote PPP end"
	PPP_DEST_IP="$IP_ADDR"

	if [ ! -r $INTERFACE ] ; then
		Error "no $INTERFACE file"
		hit_hey
		return $FAIL
	fi

	old_conf=`grep "$PPP_DEST_IP" $INTERFACE | fgrep "$PPP_HOST_IP" | grep "^ppp:"` 
	old_ip=`echo $old_conf | awk -F: '{ print $3 }'` 
	if [ "$PPP_DEST_IP" = "$old_ip" ] ; then
		message "\nPPP interface $PPP_DEST_IP-->$PPP_HOST_IP already exists. Can't add interface $PPP_HOST_IP-->$PPP_DEST_IP."
		return $FAIL
	fi

	if [ -n "$old_conf" ] && [ "$PPP_HOST_IP" = "$old_ip" ] ; then
		message "This interface already exists. The entry will be modified."
		# Get rid of old entry
		PPP_UNIT=`echo $old_conf | awk -F: '{ print $2 }'`
		sed "/^ppp:$PPP_UNIT:/d" $INTERFACE > /tmp/inface$$
		safemv /tmp/inface$$ $INTERFACE
		PPP_NM=`echo $old_conf | awk -F: '{ print $5 }'`
		PPP_NM=`echo $PPP_NM | awk  '{ print $3 }'`
		ask_mask "netmask" $PPP_NM
		PPP_NM="$NET_MASK"
	else
		PPP_UNIT=`grep "^ppp" $INTERFACE | awk -F: '{ if ($2 >= unit) unit = $2 } END {print unit }'`
		if [ ! "$PPP_UNIT" ] ; then
			PPP_UNIT=-1
		fi
		PPP_UNIT=`expr $PPP_UNIT + 1 `
		ask_mask "netmask"
		PPP_NM="$NET_MASK"
	fi
	
	if [ "$PPP_NM" ] ; then
		echo "ppp:$PPP_UNIT:$PPP_HOST_IP:/dev/ppp:$PPP_DEST_IP netmask $PPP_NM:add_ppp:" >> $INTERFACE
	else
		echo "ppp:$PPP_UNIT:$PPP_HOST_IP:/dev/ppp:$PPP_DEST_IP:add_ppp:" >> $INTERFACE
	fi
}

##############################################################################
# process ppphosts entry field which takes value
#
getfield1 (){	
	FIELD=
	fieldname=`echo $1 | awk -F: '{ print $1 }'`
	default=`echo $1 | awk -F: '{ print $2 }'`
	explain=`echo $1 | awk -F: '{ print $3 }'`
	current=$default

	if [ "$2" ] ; then
		for i in $2 
		do
		name=`echo $i | sed 's/=[a-zA-Z0-9]*//'`
		if [ "$fieldname" = "$name" ] ; then
			current=`echo $i | sed 's/[a-zA-Z]*=//'`
		fi
		done
	fi

	if [ "$forever" ] ; then
		prompt="\n$explain ("$forever") [$current]: \c"
	else
		prompt="\n$explain [$current]: \c"
	fi
	while echo "$prompt"
	do	read field
		case $field in
		[a-fA-F0-9]*) FIELD="$fieldname=$field"
				break;;
		"") if [ "$current" != "$default" ] ; then
			FIELD="$fieldname=$current"
		    fi
		    break;;
		-) if [ "$forever" ] ; then
			FIELD=
			break
		   else
			 echo "Enter a number"
		   fi
		   ;;
		*) echo "Enter a number"
		   ;;
		esac
	done
}
		
##############################################################################
# process ppphosts entry switch field
#
getfield2 (){	
	FIELD=
	fieldname=`echo $1 | awk -F: '{ print $1 }'`
	default=`echo $1 | awk -F: '{ print $2 }'`
	explain=`echo $1 | awk -F: '{ print $3 }'`
	current=$default

	if [ "$2" ] ; then
		for i in $2 
		do
		if [ "$fieldname" = "$i" ] ; then
			if [ "$default" = "on" ] ; then
				current="off"
			else
				current="on"
			fi
		fi
		done
	fi

	prompt="\n$explain (\"on\" or \"off\") [$current]: \c"
	while echo "$prompt"
	do	read field
		case $field in
		on)	field="on" 
			break;;
		off)	field="off"
			break;;
		"")	field="$current"
			break;;
		*) echo "Answer \"on\" or \"off\" "
		   ;;
		esac
	done

	if [ "$field" = "on" ] ; then
		if [ "$default" = "off" ] ; then
			FIELD=$fieldname
		else
			FIELD=
		fi
	else
		if [ "$default" = "on" ] ; then
			FIELD=$fieldname
		else
			FIELD=
		fi
	fi
}

##############################################################################
getoptions (){	

	OPTIONS=

	for i in "$IDLE" "$TOUT" "$CONF" "$TEM" "$NAK" "$MRU" "$ACCM" "$PAPTMOUT" "$DEBUG"
	do
		if [ "$i" = "$IDLE" ] ; then
			forever="\"-\" for forever"
		else
			forever=
		fi
		getfield1 "$i" "$1"
		if [ "$FIELD" ] ; then
			OPTIONS="$OPTIONS $FIELD"
		fi
	done

	for i in "$PAP" "$MGC" "$PROTCOMP" "$ACCOMP" "$IPADDR" "$IP1172" "$VJ" "$OLD"
	do
		getfield2 "$i" "$1"
		if [ "$FIELD" ] ; then
			if [ "$FIELD" = "pap" ] ; then
				warning "Add remote hosts' ID/password to local host's authentication file"
				warning "Add remote hosts' ID/password to remote hosts' authentication file"
			fi
			OPTIONS="$OPTIONS $FIELD"
		fi
	done

}

##############################################################################
conf_ppphosts() {

	# if there is no PPPHOSTS file, creat it
	if [ ! -r $PPPHOSTS ] ; then
		if [ -r /etc/inet/ppphosts.samp ] ; then
			/usr/bin/cp /etc/inet/ppphosts.samp $PPPHOSTS
		else
			echo "#ppphosts file" > $PPPHOSTS
		fi
		/usr/bin/chown bin $PPPHOSTS
		/usr/bin/chgrp bin $PPPHOSTS
		/usr/bin/chmod 644 $PPPHOSTS
	fi

	if [ ! "$1" ] ; then 
		ask_name "PPP login name"
		PPP_DEST_NAME="*$NAME"
	else
		PPP_DEST_NAME=$1
	fi

	entry=`grep "^$PPP_DEST_NAME[	 ]" $PPPHOSTS`
	if [ -n "$entry" ] ; then
		message "\nThis entry already exists. The entry will be modified" 
		# Get rid of old entry
		grep "[^\$PPP_DEST_NAME[ 	]" $PPPHOSTS|grep [\\]$ >/dev/null 2>&1
		if [ $? -eq 0 ] ; then
			sed "/^$PPP_DEST_NAME/,/[^\\]$/d" $PPPHOSTS > /tmp/ppphosts$$
		else
			sed "/^$PPP_DEST_NAME/d" $PPPHOSTS > /tmp/ppphosts$$
		fi
		safemv /tmp/ppphosts$$ $PPPHOSTS

		if [ "$1" ] ; then
			PPP_UUCP_NAME=`echo $entry | awk '{ print $3 }'`
			ask_uucpname "UUCP name" $PPP_UUCP_NAME
			if [ ! "$NAME" ] ; then
				return $OK
			fi
			PPP_UUCP_NAME="$NAME"
		else
			PPP_UUCP_NAME="-"
		fi
	
		# generate new entry
		options=`echo $entry | awk '{ for (i = 4 ; i <= NF; i++) printf "%s ", $i }'`

		getyn "\nDo you want to specify PPP negotiation parameters?"	
		case $? in
			 0) getoptions "$options"	
			    newentry="$PPP_DEST_NAME - $PPP_UUCP_NAME $OPTIONS"
			    if [ "$PPP_UUCP_NAME" = "-" ] ; then 
				current= 
				for i in "$options" 
				do
					name=`echo $i | sed 's/=[.a-zA-Z0-9]*//'`
					if [ "$name" = "remote" ] ; then
						current=`echo $i | sed 's/[a-zA-Z]*=//'`
					fi
				done
			   	ask_ip "remote IP address(\"-\" for null)" "$current"
			    	if [ $IP_ADDR ] ; then
					newentry="$newentry remote=$IP_ADDR"
			   	fi
			   fi
			   ;;
			1) newentry="$PPP_DEST_NAME - $PPP_UUCP_NAME"
			   ;;
		esac 
	else
		# creat new entry
		if [ "$1" ] ; then
			ask_uucpname "UUCP name" 
			if [ ! "$NAME" ] ; then
				return $OK
			fi
			PPP_UUCP_NAME="$NAME"
		else
			add_user $NAME
			PPP_UUCP_NAME="-"
		fi

		getyn "\nDo you want to specify PPP negotiation parameters?"	
		case $? in
			0) getoptions 
			   newentry="$PPP_DEST_NAME - $PPP_UUCP_NAME $OPTIONS"
			   if [ "$PPP_UUCP_NAME" = "-" ] ; then  
			   	ask_ip "remote IP address(\"-\" for null)" 
			    	if [ $IP_ADDR ] ; then
					newentry="$newentry remote=$IP_ADDR"
			   	fi
			   fi
			   ;;
			1) newentry="$PPP_DEST_NAME - $PPP_UUCP_NAME"
			   ;;
		esac 
	fi

	echo $newentry >> $PPPHOSTS

}

##############################################################################
conf_incoming() {
	conf_ppphosts 
}

##############################################################################
conf_hosts() {
	conf_interface 
	if [ $? -eq $FAIL ] ; then
		return $FAIL
	fi
	conf_ppphosts $PPP_DEST_IP 
	if [ -n "$PPP_UUCP_NAME" ] && [ "$PPP_UUCP_NAME" != '-' ] ; then
		getyn "\nDo you want to add an entry for the remote host to /etc/hosts?"	
		case $? in
			0)ask_name "remote PPP host name"
			  PPP_DEST_NAME="$NAME"
			  put_hosts $PPP_DEST_IP $PPP_DEST_NAME
			   ;;
			1) ;;
		esac 
	fi
}

##############################################################################
rm_login() {

	if [ ! -r "$PPPHOSTS" ] ; then
		message "$PPPHOSTS file does not exist."
		return $OK
	fi

	login=`grep "^\*" $PPPHOSTS | awk '{ print $1 }' | sed 's/*//'` 

	if [ "$login" ] ; then
		message "\nThe following PPP login accounts are configured."
	else
		message "\nThere is no configured PPP login account."
		return $OK
	fi
	echo $login

	ask_name "PPP login name to be removed"
	PPP_LOGIN="$NAME"

	old_config=`grep "^\*$PPP_LOGIN" $PPPHOSTS ` 
	if  [ -z "$old_config" ]  ; then
                echo "\nError: Could not find PPP login $PPP_LOGIN.\n"
                return  $FAIL
	fi

	getyn "Are you sure you want to remove PPP negotiation parameters for $PPP_LOGIN?"	
	case $? in
		0) grep -v "^\*$PPP_LOGIN" $PPPHOSTS > /tmp/ppphosts$$
	           safemv /tmp/ppphosts$$ $PPPHOSTS
		    ;;
		1) ;;
	esac 

	logname=`grep "^$NAME:" $PASSWORD`
	if [ "$logname" ] ; then	
		delete_user $NAME
	else
		message "There is no entry for $NAME in $PASSWORD."
		message "If $NAME is administered via NIS please take appropriate action."
	fi

}

##############################################################################
rm_host() {

	old_config=`grep "^ppp:" $INTERFACE | sed 's/ /:/g'`
	ppp_hosts=
	hosts=

	if [ "$old_config" ] ; then
		message "\nThe following PPP interfaces are configured."
	else
		message "\nThere is no configured PPP interface."
		return $OK
	fi

	for i in $old_config
	do
		LOCAL=`echo $i | awk -F: '{ print $3 }'` 
		REMOTE=`echo $i | awk -F: '{ print $5 }'` 
		REMOTE=`echo $REMOTE | awk -F: '{ print $1 }'` 
		echo "$LOCAL---> $REMOTE"
	done

	ask_ip "local IP address for the PPP interface to be removed"
	PPP_HOST_IP="$IP_ADDR"
	ask_ip "remote IP address for the PPP interface to be removed"
	PPP_DEST_IP="$IP_ADDR"

	if ( [ ! "$PPP_HOST_IP" ] || [ ! "$PPP_DEST_IP" ] ) ; then
		return $OK
	fi	
	
	old_config=`grep "$PPP_DEST_IP" $INTERFACE | fgrep "$PPP_HOST_IP" | grep "^ppp:"` 

	if [ -z "$old_config" ] ; then	
                echo "\nError: Could not find interface $PPP_HOST_IP-->$PPP_DEST_IP.\n"
                return  $FAIL
        fi

	getyn "\nAre you sure you want to remove $PPP_HOST_IP-->$PPP_DEST_IP?"	
	case $? in
		0) ;;
		1) return  $FAIL
		   ;;
	esac 

	PPP_HOST_IP=`echo $old_config | awk -F: '{ print $3 }'`
	if [ -z "$PPP_HOST_IP" ] ; then
                echo "\nError: Could not find interface with this local IP address.\n"
                return  $FAIL
        fi
	PPP_DEST_IP=`echo $old_config | awk -F: '{ print $5 }'`
	if [ -z "$PPP_DEST_IP" ] ; then
                echo "\nError: Could not find interface with this remote IP address.\n"
                return $FAIL
        fi
	PPP_DEST_IP=`echo $PPP_DEST_IP | awk  '{ print $1 }'`
        if [ -z "$PPP_DEST_IP" ] ; then
                echo "\nError: Could not find interface with this remote IP address.\n"
                return $FAIL
        fi

	if [ -r "$PPPHOSTS" ] ; then
		ppp_hosts=`grep "^$PPP_DEST_IP[         ]" $PPPHOSTS | grep -v "^#"`
	fi

	if [ -r "$HOSTS" ] ; then
		hosts=`grep "^$PPP_DEST_IP[	 ]" $HOSTS`
	fi

	# Remove all occurrences of driver in configuration files
        grep -v "^$old_config" $INTERFACE > /tmp/inface$$
	safemv /tmp/inface$$ $INTERFACE	
	if [ -n "$ppp_hosts" ] ; then
        	grep -v "^$ppp_hosts" $PPPHOSTS > /tmp/ppphosts$$
		safemv /tmp/ppphosts$$ $PPPHOSTS
	fi
	if [ -n "$hosts" ] ; then
        	egrep -v "^$hosts" $HOSTS > /tmp/hosts$$
		safemv /tmp/hosts$$ $HOSTS
	fi

	# Remove /etc/inet/ppphosts and /etc/inet/pppauth files
	num_units=`grep "^ppp:" $INTERFACE | grep "add_ppp" | wc -l`
	if [ $num_units -eq 0 ] ; then
		if [ -r "$PPPHOSTS" ] ; then
			/usr/bin/rm $PPPHOSTS
		fi
		if [ -r "$PPPAUTH" ] ; then
			/usr/bin/rm $PPPAUTH
		fi
	fi
}

##############################################################
confppp() {

	while true
	do
		TOP="Configure PPP Hosts"
		sub_menu "Add/modify PPP hosts" "Remove PPP hosts" "$TOP"
		case $? in
			1) conf_hosts ;;
			2) rm_host ;;
			0) return $OK
		esac
	done
}

##############################################################
confin() {

	while true
	do
		TOP="Configure Incoming PPP Parameters"
		sub_menu "Add/modify incoming PPP setup" "Remove incoming PPP setup" "$TOP"
		case $? in
			1) conf_incoming ;;
			2) rm_login ;;
			0) return $OK
		esac
	done
}

#######################################################
add_auth() {

	while true
	do
		TOP="Add/modify PPP authentication ID/password"
		sub_menu "Local host" "Remote host" "$TOP"
		case $? in
			1) add_local ;;
			2) add_remote ;;
			0) return $FAIL
		esac
	done
}

#######################################################
rm_auth() {

	while true
	do
		TOP="Remove PPP authentication ID/password"
		sub_menu "Local host" "Remote host" "$TOP"
		case $? in
			1) rm_local ;;
			2) rm_remote ;;
			0) return $FAIL
		esac
	done
}

#######################################################
edit_auth() {

	while true
	do
		TOP="Configure PPP authentication parameters"
		sub_menu "Add/modify ID/password" "Remove ID/password" "$TOP"
		case $? in
			1) add_auth ;;
			2) rm_auth ;;
			0) return $FAIL
		esac
	done
}

##############################################################################
#main

uid=`/bin/id|awk '{ print $1}`
uid=`echo $uid| sed 's/uid=//'| sed 's/(.*//`
if [ "$uid" -ne 0 ] ; then
	message "Must be super user to use $0"
	exit $FAIL	
fi

c1="Configure PPP hosts" 
c2="Configure incoming PPP parameters" 
c3="Configure PPP authentication parameters"

menu_prompt="\n\nPPP Configuration Menu\n\n1) $c1\n2) $c2\n3) $c3\nq) Quit\n\nEnter choice: \c"

while echo "$menu_prompt"
do read cmd
	case $cmd in
	1) confppp ;;
	2) confin ;;
	3) edit_auth ;;
	q) exit $OK ;;
	*) echo "illegal choice" ;;
	esac
done
