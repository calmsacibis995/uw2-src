#ident	"@(#)lp.admin:printers/operations/startfailed.t	1.2.4.1"
#ident  "$Header: startfailed.t 2.0 91/07/12 $"

title="Start printer services task"

altslks=true

row=15
columns=55

begrow=distinct
begcol=distinct

text="

    A failure has occurred while attempting to
    start the printer service.

`readfile $error`

    Press CANCEL to quit.
"

name=CANCEL
button=14
action=CLOSE `getfrm`
