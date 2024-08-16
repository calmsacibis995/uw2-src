#ident	"@(#)lp.admin:printers/classes/Menu.class.ch	1.2.5.3"
#ident  "$Header: Menu.class.ch 2.0 91/07/12 $"

menu=Choices
lifetime=shortterm
multiselect=true
framemsg="MARK choices then press ENTER"

`set -l name_1="/tmp/lp.cl$VPID";
ls /etc/lp/classes > $name_1;
`
close=`/usr/bin/rm -f $name_1;
	unset -l $name_1`

done=`getitems " "|set -l Form_Choice`close

`/usr/bin/sort $name_1 | regex '^(.*)$0$' 'name=$m0'`
