#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ypcmd:ypinit.sh	1.6.10.24"
#ident  "$Header: $"
#!/sbin/sh

#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#	PROPRIETARY NOTICE (Combined)
#
# This source code is unpublished proprietary information
# constituting, or derived under license from AT&T's UNIX(r) System V.
# In addition, portions of such source code were derived from Berkeley
# 4.3 BSD under license from the Regents of the University of
# California.
#
#
#
#	Copyright Notice 
#
# Notice of copyright on this source code product does not indicate 
#  publication.
#
#	(c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
#	(c) 1990,1991  UNIX System Laboratories, Inc.
#          All rights reserved.
#

# set -xv
yproot_dir=/var/yp
yproot_exe=/usr/sbin/yp
hf=/tmp/ypservers.$$
XFR=${YPXFR-ypxfr}
maps=`cat $yproot_dir/YPMAPS | grep -v "#"`

clientp=F
masterp=F
slavep=F
host=""
def_dom=""
master=""
got_host_list=F
first_time=T
exit_on_error=F
errors_in_setup=F
fatal=0
badhosts_file=/tmp/.badhosts.$$

lecho()
{
	pfmt -s nostd -g ypinit:$1 "$2" $3 $4 $5
}
error()
{
	pfmt -s error -g ypinit:$1 "$2" $3 $4 $5
}
warning()
{
	pfmt -s warn -g ypinit:$1 "$2" $3 $4 $5
}
bad_host()
{

	echo $1 >> $badhosts_file
#	warning 17 "The following machines are not responding.\n\n\tPlease verify that each machine has an entry in /etc/hosts \n\tand is reachable over the network.\n\n" $h $h
#	warning 17 "machine: %s is not responding.\n\n\tPlease verify that %s has an entry in /etc/hosts \n\tand is reachable over the network.\n\n" $h $h
}
Usage() {
lecho 1 "usage:\n\
    ypinit -c\n\
    ypinit -m\n\
    ypinit -s master_server\n\
where:\n\
    -c is used to set up a nis client\n\
    -m is used to build a master nis server data base\n\
    -s is used for a slave data base and\n\
        master_server must be an existing reachable nis server.\n"
}
setdomain()
{
	def_dom=""
	while [ -z "$def_dom" ]
	do
		lecho 2 "Please enter the domain name or q to quit: "
		read def_dom
		if [ "$def_dom" = "q" ]
		then
			exit 1
		fi
		if [ -n "$def_dom" ]
		then
			lecho 3 "Is %s correct? [y/n: y] " $def_dom
			read ans
			if [ "$ans" = "n" -o "$ans" = "N" ]
			then
				def_dom=""
			fi
		fi
	done
}
set_initd_nis()
{
	chmod 0644 /etc/init.d/nis
	ed - /etc/init.d/nis << EOF > /dev/null 2>&1
/^domain=/s/\".*/\"$1\"/
/^isserver=/s/=[0-9]*/=$2/
/^ismaster=/s/=[0-9]*/=$3/
/^startypbind=/s/=[0-9]*/=1/
w
q
EOF
	chmod 0444 /etc/init.d/nis
	echo "domainname $def_dom" > /etc/rc2.d/S51domain
	chmod 0755 /etc/rc2.d/S51domain
}
set_libns()
{
	if [ ! -f /usr/lib/ns.so ]; then
		/sbin/ln -s /usr/lib/.ns.so /usr/lib/ns.so
	fi
	if [ ! -f /usr/lib/ns.so.1 ]; then
		/sbin/ln -s /usr/lib/.ns.so /usr/lib/ns.so.1
	fi
}
set_netconfig()
{
grep tcpip_nis /etc/netconfig > /dev/null 2>&1
if [ $? = 1 ]; then
	nawk '  BEGIN { OFS="\t" }
			{
			   if (match($7, "/usr/lib/tcpip.so")) 
				  sub("/usr/lib/tcpip.so", "/usr/lib/tcpip_nis.so,/usr/lib/tcpip.so", $7);
			  print $0
			} ' < /etc/netconfig > /tmp/net$$

	if [ $? -eq 0 ]; then
		mv /tmp/net$$ /etc/netconfig
	else
		errors_in_setup=T
		error 42 "Unable to add tcpip_nis.so to /etc/netconfig\n"
	fi
fi
}
startnis()
{
	#
	# Make sure the network is up
	#
	if ifconfig -a inet > /dev/null 2>&1
	then
		lecho 4 "Starting NIS.... "
		sh /etc/init.d/nis start
		lecho 5 "Done\n"
	fi
}

ckhost()
{
	#
	# If the network is up, see if we can
	# ping the host
	#
	case $1 in
                \**|+*) return 1 ;;
        esac

	found=0
	if ifconfig -a inet > /dev/null 2>&1
	then
		ping $1 3 > /dev/null 2>&1
		if [ $? = 0 ]
		then
			found=1
		fi
		return $found
	fi
	#
	# Now check to see if the host is in /etc/hosts
	#
	found=`cat /etc/hosts | sed '/^#/d' | nawk '
		BEGIN {found=0}
			host == $2 {found=1; exit 0}
		END {print found}' host=$1`
	return $found
}

PATH=/bin:/usr/bin:/usr/etc:/usr/sbin:$yproot_exe:$PATH
export PATH 

case $# in
1)	case $1 in
	-c)	clientp=T;;
	-m)	masterp=T;;
	*)	Usage
		exit 1;;
	esac;;

2)	case $1 in
	-s)	slavep=T; master=$2;;
	*)	Usage
		exit 1;;
	esac;;

3)	case $1 in
	-c)	clientp=T;;
	*)	Usage
		exit 1;;
	esac;;

*)	Usage
	exit 1;;
esac

uid=`id | cut -d= -f2 | cut -d\( -f1`
if [ "$uid" != "0" ]; then
	error 6 "You have to be the NIS Administrator to run ypinit\n"
	exit 1
fi

lecho 7 "Shutting Down NIS.... "
sh /etc/init.d/nis stop
lecho 5 "Done\n"

host=`uname -n`

if [ $? -ne 0 ]
then 
	error 8 "\nCan not get local host's name.  Please check your path.\n"
	exit 1
fi

if [ -z "$host" ]
then
	error 9 "\nThe local host's name has not been set.  Please set it.\n"
	exit 1
fi

def_dom=`domainname`

if [ $? -ne 0 ]
then 
	error 10 "\nCan not get local host's domain name.  Please check your path.\n"
	exit 1
fi

if [ -z "$def_dom" ]
then
	lecho 11 "\nThe local host's domain name has not been set.  Please set it.\n"
	setdomain
else
	lecho 12 "\nThe local host's domain name has been set to '%s'\n" $def_dom
	lecho 13 "Is this correct? [y/n: y] "
	read ans

	case $ans in
	y|Y|"")
		;;
	*)
		setdomain
		;;
	esac
fi

domainname $def_dom
real_def_dom=$def_dom
def_dom=`ypalias -d $def_dom`
ypservers_map=`ypalias ypservers`
domain_dir="$yproot_dir""/""$def_dom" 
binding_dir="$yproot_dir""/binding/""$def_dom"
binding_file="$yproot_dir""/binding/""$def_dom""/ypservers"

if [ ! -d $yproot_dir -o -f $yproot_dir ]
then
    error 14 "\n\
The directory %s does not exist.  Re-install the NIS package.\n" $yproot_dir
	exit 1
fi

# add domainname and ypservers aliases to aliases file
echo ypservers $ypservers_map >> $yproot_dir/aliases
echo $real_def_dom $def_dom >> $yproot_dir/aliases
sort $yproot_dir/aliases | grep -v '#ident' | uniq > /tmp/.ypaliases 
mv /tmp/.ypaliases $yproot_dir/aliases

if [ ! -d "$yproot_dir"/binding ]
then
	mkdir "$yproot_dir"/binding
	if [ -x /sbin/chlvl ]
	then
		chlvl SYS_PUBLIC "$yproot_dir"/binding
	fi
#else
#	rm -rf "$yproot_dir"/binding/*
fi

if [ ! -d  $binding_dir ]
then
	mkdir  "$binding_dir"
	if [ -x /sbin/chlvl ]
	then
		chlvl SYS_PUBLIC "$binding_dir"
	fi
fi

if [ $slavep = F ]
then
	rm -f $binding_file
	while [ $got_host_list = F ]; do
		lecho 15 "\n\
In order for NIS to operate sucessfully, we have to construct a\n\
list of the NIS servers. Please continue to add the names for\n\
NIS servers in order of preference, one per line. When you\n\
are done with the list, type a <control D>.\n"
		if [ $masterp = T ]
		then
			echo $host > $hf
			lecho 16 "\tnext host to add:  %s\n" $host
		elif [ -f $binding_file ]
		then
			if [ $first_time = T ]
			then
				for h in `cat $binding_file`
				do
					echo $h >> $hf
					lecho 17 "\tnext host to add:  %s\n" $h
				done
			fi
		fi
		while true
		do
			lecho 18 "\tnext host to add:  "

			while read h
			do
				if [ -n "$h" ]; then
					if ckhost $h
					then
						bad_host $h
					else
						echo $h >> $hf
					fi
				fi
				lecho 18 "\tnext host to add:  "
			done
			if [ -s $hf ] || [ -s $badhosts_file ]
			then
				break
			fi
			echo ""
		done

if [ -s $hf ]
then 
	if [ -s $badhosts_file ]
	then
	echo "\n \n"
	warning 43 "The following machines could not be contacted over the network."
	echo "\n"
	for i in `cat $badhosts_file`
	do
		echo "\t \t \t$i"
	done
	echo "\n"
	warning 44 " Although NIS will initiate successfully, it is recommended you \n\t  verify that each machine has an entry in /etc/hosts and is \n\t  reachable over the network and then re-run ypinit.\n\n" $h $h
	fi
else
	if [ -s $badhosts_file ]
	then
	echo "\n \n"
	error 43 "The following machines could not be contacted over the network."
	echo "\n"
	for i in `cat $badhosts_file`
	do
		echo "\t \t \t$i"
	done
	echo "\n"
	error 45 "\tNIS will NOT be started, since none of the machines \n\tentered as NIS servers can be reached over the network.\n \n\tPlease verify that each NIS server has an entry in /etc/hosts \n\tand is reachable over the network and then re-run ypinit.\n\n" $h $h
		exit 1
	fi
fi
		echo ""
		lecho 19 "The current valid list of nis servers looks like this:\n"
		echo ""

		cat $hf
		echo ""
		lecho 13 "Is this correct?  [y/n: y]  "
		read hlist_ok

		case $hlist_ok in
		n*)	got_host_list=F
			first_time=F
			rm -f $hf
			rm -f $badhosts_file
			lecho 20 "Let's try the whole thing again...\n";;
		N*)	got_host_list=F
			first_time=F
			rm -f $hf
			rm -f $badhosts_file
			lecho 20 "Let's try the whole thing again...\n";;
		*)	got_host_list=T;;
		esac
	done
	cp  $hf $binding_file
	if [ -x /sbin/chlvl ]
	then
		chlvl SYS_PUBLIC $binding_file
	fi
fi

#
# If client only, we are done
# 	our purpose was just to set up the binding file
#
if [ $clientp = T ]
then
	set_initd_nis $def_dom 0 0
	set_netconfig
	set_libns
	lecho 21 "%s has been set up as a nis client.\n" $host
	rm -f $hf
	startnis
	exit 1
fi

if [ $slavep = T ]
then
	if [ $host = $master ]
	then
		error 22 "\
The host specified should be a running master nis server, not this machine.\n"
		exit 1
	fi

	if ckhost $master
	then
	echo "\n \n"
	error 46 " The NIS master server, %s, could not be \n\tcontacted over the network." $master
	echo "\n"
	error 47 "\tNIS will NOT be started. \n \n\tPlease verify that server, %s, has an entry in /etc/hosts \n\tand is reachable over the network and then re-run ypinit.\n\n" $master
		exit 1
	fi
fi

for dir in $yproot_dir/$def_dom
do

	if [ -d $dir ]; then
		lecho 23 "Can we destroy the existing %s and its contents? [y/n: n]  " $dir
		read kill_old_dir

		case $kill_old_dir in
		y*)	rm -r -f $dir

			if [ $?  -ne 0 ]
			then
			error 24 "Can not clean up old directory %s.  Fatal error.\n" $dir
				exit 1
			fi;;

		Y*)	rm -r -f $dir

			if [ $?  -ne 0 ]
			then
			error 24 "Can not clean up old directory %s.  Fatal error.\n" $dir
				exit 1
			fi;;

		*)    lecho 25 "OK, please clean it up by hand and start again.  Bye\n"
			exit 0;;
		esac
	fi

	mkdir $dir

	if [ $?  -ne 0 ]
	then
		error 26 "Can not make new directory %s.  Fatal error.\n" $dir
		exit 1
	fi

done

echo ""
lecho 27 "The nis domain directory is %s""/""%s\n" $yproot_dir $def_dom

if [ $slavep = T ]
then
	lecho 28 "\nThe following NIS databases will be copied from the master server:\n"
else
	lecho 29 "\nThe following NIS databases will be created for this domain:\n"
fi
echo $maps | awk '{
	printf("\n\t");
	for (i=1; i <= NF; i++){
		printf("%-20s", $i);
		if ((i % 3) == 0)
			printf("\n\t");
	}
	printf("\n");
}'
echo ""
lecho 30 "Are these correct? [y/n: y] "
read ans
if [ "$ans" = "n" -o "$ans" = "N" ]
then
	lecho 31 "\nPlease edit the /var/yp/YPMAPS file and start again.\n"
	exit 1
fi

if [ $slavep = T ]
then
	set_initd_nis $def_dom 1 0
	set_netconfig
	set_libns
	lecho 32 "\nThere will be no further questions.\n\
%s has been set up as a nis slave server.\n" $host
	echo "master=$master" > $yproot_dir/xfrmaps
	echo "maps=\"$maps\"" >> $yproot_dir/xfrmaps
	startnis
	exit 0
fi

lecho 33 "\nDo you want this procedure to quit on non-fatal errors? [y/n: n] "
read doexit

case $doexit in
y*)	exit_on_error=T;;
Y*)	exit_on_error=T;;
*)	;;
esac

#
# This must be a NIS master so create NIS maps
#
rm -f $yproot_dir/*.time

lecho 35 "\nThere will be no further questions. The remainder of the \
procedure\nmay take several minutes.\n"

lecho 36 "Building %s/%s/ypservers...\n" $yproot_dir $def_dom
makedbm $hf $yproot_dir/$def_dom/$ypservers_map

if [ $?  -ne 0 ]
then
	error 37 "\
Could not build nis data base %s/%s/%s.\n" $yproot_dir $def_dom $ypservers_map
	errors_in_setup=T

	if [ $exit_on_error = T ]
	then
		rm -f $hf
		exit 1
	fi
fi

rm -f $hf

in_pwd=`pwd`
cd $yproot_dir
lecho  38 "Running %s/Makefile\n" $yproot_dir

# ypbuild is used instead of the regular make command
# because /usr/ccs/bin/make cannot inherit privileges
# in Enhanced Security environment.

/var/yp/ypbuild MAKE=/var/yp/ypbuild SHELL=/sbin/sh NOPUSH=1 

if [ $?  -ne 0 ]
then
	error 39 "Error running Makefile.\n"
	errors_in_setup=T
		
	if [ $exit_on_error = T ]
	then
		exit 1
	fi
fi

set_initd_nis $def_dom 1 1
set_netconfig
set_libns
cd $in_pwd
echo ""

if [ $errors_in_setup = T ]
then
lecho 40 "%s has been set up as a nis master server \
with errors.  Please remember\nto figure out what went wrong, \
and fix it.\n\n" $host
else
lecho 41 "%s has been set up as a nis master server \
without any errors.\n\n" $host
	startnis
fi

exit 0
