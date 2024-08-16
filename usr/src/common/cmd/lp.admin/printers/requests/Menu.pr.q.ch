#ident	"@(#)lp.admin:printers/requests/Menu.pr.q.ch	1.5.3.2"
#ident  "$Header: Menu.pr.q.ch 2.0 91/07/12 $"

menu=Choices
lifetime=shortterm
multiselect=true
framemsg="MARK choices then press ENTER"


`set -l name_1="/tmp/lp.n1$VPID";
if [ -n "$TFADMIN" ]; then $TFADMIN lpstat -oall | fmlcut -d'-' -f1  > $name_1;
else lpstat -oall | fmlcut -d'-' -f1  > $name_1; fi;
if [ -s $name_1 ];
then
	echo "all" >> $name_1;
	echo "init=true";
else
	echo "init=false";
	message -b "There are no printers with print requests";
	/usr/bin/rm -f  $name_1;
fi`

close=`/usr/bin/rm -f  $name_1;
	unset -l $name_1`

done=`getitems " "|set -l Form_Choice`close

`/usr/bin/sort -u $name_1 | regex '^(.*)$0$' 'name=$m0'`
