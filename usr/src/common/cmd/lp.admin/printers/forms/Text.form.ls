#ident	"@(#)lp.admin:printers/forms/Text.form.ls	1.4.3.3"
#ident  "$Header: Text.form.ls 2.0 91/07/12 $"

#	Module Name: Text.form.ls

title="List Form Attributes" 

lifetime=shortterm

`set -l list="/tmp/lp.fm$VPID";
	set -l flist="/tmp/lp.fml$VPID";`

`echo "$ARG1" | tr -s ',' ' ' | set -l flist;
       shell "
          for f in $flist
          do
  	echo "Form: " \$f >> $list;
	$TFADMIN lpforms -l -f\$f >> $list;
  	echo " "  >> $list;
	if [ -s /etc/lp/forms/\$f/alert.sh ];
	then 
	$TFADMIN lpforms -f\$f -Alist >> $list;
  	echo " "  >> $list;
	fi;
	if [ -s /etc/lp/forms/\$f/allow ];
	then 
	echo "Users allowed: " >> $list; 
	cat /etc/lp/forms/\$f/allow | tr '\012' ' ' >> $list;
	echo " " | tr ' ' '\012' >> $list;
	fi;
	if [ -s /etc/lp/forms/\$f/deny ];
	then 
	echo "Users denied: " >> $list;
	cat /etc/lp/forms/\$f/deny | tr '\012' ' ' >> $list;
	echo " " | tr ' ' '\012' >> $list;
	fi;
	echo " " >> $list;
        done 
	" > $error`

close=`/usr/bin/rm -f $list;
	/usr/bin/rm -f $flist;
	unset -l list -l flist;`
done=`/usr/bin/rm  -f $list;
	/usr/bin/rm -f  $flist;
	unset -l list -l flist;`close

begrow=36
begrow=1
rows=18
scroll=true
columns=50
text="`readfile \$list`"
