#ident	"@(#)oamintf:common/cmd/oamintf/interface/oam.cmd	1.2.4.4"
#ident  "$Header: oam.cmd 2.0 91/07/12 $"

name=goto
action=NOP

#name=cancel
#action=NOP

#name=cleanup
#action=NOP

name=frm-mgmt
action=NOP

#name=help
#action=NOP

name=next-frm
action=NOP

name=prev-frm
action=NOP

name=unix-system
action=NOP

#name=update
#action=NOP

name=system
action=`/usr/sbin/whodo -h |
	/usr/bin/nawk ' { if ( NF == 0 ) login = ""
			  if ( NF == 3 ) login = $2
			  if ( NF == 4 && $2 == pid ) {
			  	print login
				exit 0
			  }
			}' pid=${VPID} - | 
	set -l LOGIN; 
	run su ${LOGIN} -c "echo use CTL-D to exit; sh"`nop

