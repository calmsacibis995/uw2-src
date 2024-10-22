#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#

#ident	"@(#)ucb:common/ucbcmd/sendmail/cf/main.mc	1.5"
#ident	"$Header: $"


#		PROPRIETARY NOTICE (Combined)
#
#This source code is unpublished proprietary information
#constituting, or derived under license from AT&T's UNIX(r) System V.
#In addition, portions of such source code were derived from Berkeley
#4.3 BSD under license from the Regents of the University of
#California.
#
#
#
#		Copyright Notice 
#
#Notice of copyright on this source code product does not indicate 
#publication.
#
#	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#	          All rights reserved.

############################################################
#
#	Sendmail configuration file for "MAIN MACHINES"
#
#	You should install this file as /etc/ucbmail/sendmail.cf
#	if your machine is the main (or only) mail-relaying
#	machine in your domain.  Then edit the file to
#	customize it for your network configuration.
#
#
#

###	local info

# my official hostname
# You have two choices here.  If you want the gateway machine to identify
# itself as the DOMAIN, use this line:
Dj$m
# If you want the gateway machine to appear to be INSIDE the domain, use:
#Dj$w.$m
# Unless you are using sendmail.mx (or have a fully-qualified hostname), use:
#Dj$w

# major relay mailer - typical choice is "ddn" if you are on the
# Defense Data Network (e.g. Arpanet or Milnet)
DMsmartuucp

# major relay host: use the $M mailer to send mail to other domains
ifdef(`m4GATEWAY',, `define(m4GATEWAY,ddn-gateway)')dnl
DR m4GATEWAY
CR m4GATEWAY

# If you want to pre-load the "mailhosts" then use a line like
# FS /usr/lib/mailhosts
# and then change all the occurences of hostlookup on the RHS
# to be $=S on the LHS instead.
# Otherwise, the default is to use the hosts.byname map if YP
# is running (or else the /etc/hosts file if no YP).
# Need the hostlookup map to replace %y

Khostlookup host -f -m -a.

# valid top-level domains (default passes ALL unknown domains up)
CT arpa com edu gov mil net org 
CT us de fr jp kr nz il uk no au fi nl se ca ch my dk ar

# options that you probably want on a mailhost:

# checkpoint the queue after this many receipients
OC10

# refuse to send tiny messages to more than these recipients
Ob10

include(sunbase.m4)

include(uucpm.m4)

include(ddnm.m4)

# "Smart" UUCP mailer: Uses UUCP transport but domain-style naming
Msmartuucp, P=/usr/bin/uux, F=CmsDFMhuU, S=22, R=22, 
	A=uux - -r $h!rmail ($u)
	

############################################################
#
#		RULESET ZERO
#
#	This is the ruleset that determines which mailer a name goes to.

# Ruleset 30 just calls rulesets 3 then 0.
S30
R$*			$: $>3 $1			First canonicalize
R$*			$@ $>0 $1			Then rerun ruleset 0

S0
# On entry, the address has been canonicalized and focused by ruleset 3.
# Handle special cases.....
R@			$#local $:$n			handle <> form
# For numeric spec, you can't pass spec on to receiver, since rcvr's
# are not smart enough to know that [x.y.z.a] is their own name.
R<@[$+]>:$*		$:$>9 <@[$1]>:$2		Clean it up, then...
R<@[$+]>:$*		$#ether $@[$1] $:$2		numeric internet spec
R<@[$+]>,$*		$#ether $@[$1] $:$2		numeric internet spec
R$*<@[$+]>		$#ether $@[$2] $:$1		numeric internet spec

# resolve the local hostname to "LOCAL".
R$*<$*$=w.LOCAL>$*	$1<$2LOCAL>$4			thishost.LOCAL
R$*<$*$=w.uucp>$*	$1<$2LOCAL>$4			thishost.uucp
R$*<$*$=w>$*		$1<$2LOCAL>$4			thishost

# Mail addressed explicitly to the domain gateway (us)
R$*<@LOCAL>		$@$>30$1			strip our name, retry
R<@LOCAL>:$+		$@$>30$1			retry after route strip

# deliver to known ethernet hosts explicitly specified in our domain
#R$*<@$%y.LOCAL>$*	$#ether $@$2 $:$1<@$2>$3	user@host.dom.com
# Substitute SUN-style %y (above) with hostlookup
R$*<@$+.LOCAL>$*	$: $1<@$(hostlookup $2 $).LOCAL>$3	canonicalize
R$*<@$+..LOCAL>$*	$#ether $@$2 $:$1<@$2>$3	remove trailing dot

# etherhost.uucp is treated as etherhost.$m for now.
# This allows them to be addressed from uucp as foo!dom!etherhost!user.
#R$*<@$%y.uucp>$*	$#ether $@$2 $:$1<@$2>$3	user@etherhost.uucp
# Substitute SUN-style %y (above) with hostlookup
R$*<@$+.uucp>$*	$: $1<@$(hostlookup $2 $).uucp>$3	canonicalize
R$*<@$+..uucp>$*	$#ether $@$2 $:$1<@$2>$3	remove trailing dot

# Explicitly specified names in our domain -- that we've never heard of
R$*<@$*.LOCAL>$*	$#error $:Never heard of host $2 in domain $m

# Clean up addresses for external use -- kills LOCAL, route-addr ,=>: 
R$*			$:$>9 $1			Then continue...

# resolve UUCP-style names
R<@$-.uucp>:$+		$#uucp  $@$1 $:$2		@host.uucp:...
R$+<@$-.uucp>		$#uucp  $@$2 $:$1		user@host.uucp

# Pass other valid names up the ladder to our forwarder
#R$*<@$*.$=T>$*		$#$M    $@$R $:$1<@$2.$3>$4	user@domain.known

# Replace following with above to only forward "known" top-level domains
R$*<@$*.$+>$*		$#$M    $@$R $:$1<@$2.$3>$4	user@any.domain

# if you are on the DDN, then comment-out both of the the lines above
# and use the following instead:
#R$*<@$*.$+>$*		$#ddn $@ $2.$3 $:$1<@$2.$3>$4	user@any.domain

# All addresses in the rules ABOVE are absolute (fully qualified domains).
# Addresses BELOW can be partially qualified.

# deliver to known ethernet hosts
#R$*<@$%y>$*		$#ether $@$2 $:$1<@$2>$3	user@etherhost
# Substitute SUN-style %y (above) with hostlookup
R$*<@$+>$*		$: $1<@$(hostlookup $2 $)>$3	canonicalize
R$*<@$+.>$*		$#ether $@$2 $:$1<@$2>$3	remove trailing dot

# other non-local names have nowhere to go; return them to sender.
R$*<@$+.$->$*		$#error $:Unknown domain $3
R$*<@$+>$*		$#error $:Never heard of $2 in domain $m
R$*@$*			$#error $:I don't understand $1@$2

# Local names with % are really not local!
R$+%$+			$@$>30$1@$2			turn % => @, retry

# everything else is a local name
R$+			$#local $:$1			local names
