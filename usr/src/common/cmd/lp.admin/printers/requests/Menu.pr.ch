#ident	"@(#)lp.admin:printers/requests/Menu.pr.ch	1.7.3.3"
#ident  "$Header: Menu.pr.ch 2.0 91/07/12 $"

menu="Choices" 
lifetime=shortterm
multiselect=true
framemsg="MARK choices then press ENTER"

close=`/usr/bin/rm -f $name_1;
	unset -l list;
	unset -l $name_1`

done=`getitems " "|set -l Form_Choice`close


`set -l name_1="/tmp/lp.n1$VPID";
if [ -z $ARG1 ];
	then set -l list="all";
	else set -l list=$ARG1;
fi;
if [ -n "$TFADMIN" ]; 
then 
	$TFADMIN lpstat -o$list | grep 'eld' | cut -d' ' -f1 >  $name_1;
else

	         lpstat -o$list | grep 'eld' | cut -d' ' -f1 >  $name_1;
fi;
if [ -s $name_1 ];
then
	echo "all" >> $name_1;
	echo "init=true";
else
	echo "init=false";
	message -b "There are no print requests on hold.";
	/usr/bin/rm -f $name_1;
fi`

`/usr/bin/sort $name_1 | regex '^(.*)$0$' 'name=$m0'`
