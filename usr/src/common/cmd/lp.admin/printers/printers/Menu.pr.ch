#ident	"@(#)lp.admin:printers/printers/Menu.pr.ch	1.4.3.2"
#ident  "$Header: Menu.pr.ch 2.0 91/07/12 $"

menu=Choices
lifetime=shortterm
multiselect=true
framemsg="MARK choices then press ENTER"

`set -l name_1="/tmp/lp.n1$VPID";
ls /etc/lp/printers > $name_1;
if [ -s $name_1 ];
then
	echo "all" >> $name_1;
	echo "init=true";
else
	echo "init=false";
	message -b "There are no printers available";
	/usr/bin/rm -f $name_1;
fi`

close=`/usr/bin/rm -f $name_1;
	unset -l $name_1`

done=`getitems " "|set -l Form_Choice`close

`/usr/bin/sort $name_1 | regex '^(.*)$0$' 'name=$m0'`
