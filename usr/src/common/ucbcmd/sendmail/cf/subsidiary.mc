#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#

#ident	"@(#)ucb:common/ucbcmd/sendmail/cf/subsidiary.mc	1.9"
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

###########################################################
#
#	SENDMAIL CONFIGURATION FILE FOR SUBSIDIARY MACHINES
#
#	You should install this file as /etc/ucbmail/sendmail.cf
#	if your machine is a subsidiary machine (that is, some
#	other machine in your domain is the main mail-relaying
#	machine).  Then edit the file to customize it for your
#	network configuration.
#
#
#

# local UUCP connections -- not forwarded to mailhost
CV

# my official hostname
Dj$w.$m

# major relay mailer
DMether

# major relay host
DRmailhost
CRmailhost

#################################################
#
#	General configuration information

# local domain names
#
# These can now be set from the domainname system call.
# If your YP domain is different from the domain name you would like to have
# appear in your mail headers, edit them to be your mail domain name.
# Note that the first component of the YP domain name is stripped off unless
# it begins with a dot or a plus sign.
# DmPodunk.EDU
# CmPodunk.EDU
#
# The Dm value is what is set in outgoing mail.  The Cm value is what is
# accepted in incoming mail.  usually these are the same, but you might
# want to have more than one Cm line to recognize more than one domain 
# name during a transition.

# known hosts in this domain are obtained from gethostbyname() call

# Version number of configuration file
DVSMI-4.1

# If you want to pre-load the "mailhosts" then use a line like
# FS /usr/lib/mailhosts
# and then change all the occurences of hostlookup on the RHS
# to be $=S on the LHS instead.
# Otherwise, the default is to use the hosts.byname map if YP
# is running (or else the /etc/hosts file if no YP).
# Need the hostlookup map to replace %y

Khostlookup host -f -m -a.

###   Standard macros

# name used for error messages
DnMailer-Daemon
# UNIX header format
DlFrom $g  $d
# delimiter (operator) characters
Do.:%@!^=/[]
# format of a total name
Dq$g$?x ($x)$.
# SMTP login message
De$j Sendmail $v/$V ready at $b

###   Options

# Remote mode - send through server if mailbox directory is mounted
OR
# location of alias file
OA/etc/ucbmail/aliases
# default delivery mode (deliver in background)
Odbackground
# rebuild the alias file automagically
OD
# temporary file mode -- 0600 for secure mail, 0644 for permissive
OF0600
# default GID
Og1
# location of help file
OH/etc/ucbmail/sendmail.hf
# log level
OL9
# default messages to old style
Oo
# Cc my postmaster on error replies I generate
OPPostmaster
# queue directory
OQ/var/spool/mqueue
# read timeout for SMTP protocols
Or15m
# status file -- none
OS/usr/ucblib/sendmail.st
# queue up everything before starting transmission, for safety
Os
# return queued mail after this long
OT3d
# default UID
Ou1

###   Message precedences
Pfirst-class=0
Pspecial-delivery=100
Pjunk=-100

###   Trusted users
T root daemon uucp

###   Format of headers 
H?P?Return-Path: <$g>
HReceived: $?sfrom $s $.by $j ($v/$V)
	id $i; $b
H?D?Resent-Date: $a
H?D?Date: $a
H?F?Resent-From: $q
H?F?From: $q
H?x?Full-Name: $x
HSubject:
H?M?Resent-Message-Id: <$t.$i@$j>
H?M?Message-Id: <$t.$i@$j>
HErrors-To:

###########################
###   Rewriting rules   ###
###########################


#  Sender Field Pre-rewriting
S1
# None needed.

#  Recipient Field Pre-rewriting
S2
# None needed.

# Name Canonicalization

# Internal format of names within the rewriting rules is:
# 	anything<@host.domain.domain...>anything
# We try to get every kind of name into this format, except for local
# names, which have no host part.  The reason for the "<>" stuff is
# that the relevant host name could be on the front of the name (for
# source routing), or on the back (normal form).  We enclose the one that
# we want to route on in the <>'s to make it easy to find.
# 
S3

# handle "from:<>" special case
R<>			$@@				turn into magic token

# basic textual canonicalization
R$*<$+>$*		$2				basic RFC822 parsing

# make sure <@a,@b,@c:user@d> syntax is easy to parse -- undone later
R@$+,$+:$+		@$1:$2:$3			change all "," to ":"
R@$+:$+			$@$>6<@$1>:$2			src route canonical

R$+:$*;@$+		$@$1:$2;@$3			list syntax
R$+@$+			$:$1<@$2>			focus on domain
R$+<$+@$+>		$1$2<@$3>			move gaze right
R$+<@$+>		$@$>6$1<@$2>			already canonical

# convert old-style names to domain-based names
# All old-style names parse from left to right, without precedence.
R$-!$+			$@$>6$2<@$1.uucp>		uucphost!user
R$-.$+!$+		$@$>6$3<@$1.$2>			host.domain!user
R$+%$+			$@$>3$1@$2			user%host

#  Final Output Post-rewriting 
S4
R$+<@$+.uucp>		$2!$1				u@h.uucp => h!u
R$+			$: $>9 $1			Clean up addr
R$*<$+>$*		$1$2$3				defocus


#  Clean up an name for passing to a mailer
#  (but leave it focused)
S9
R@			$@$n				handle <> error addr
R$*<$*LOCAL>$*		$1<$2$m>$3			change local info
R<@$+>$*:$+:$+		<@$1>$2,$3:$4			<route-addr> canonical


#######################
#   Rewriting rules

# special local conversions
S6
R$*<@$*$=m>$*		$1<@$2LOCAL>$4			convert local domain

# Local and Program Mailer specification

Mlocal,	P=/usr/bin/rmail, F=flsDFMmnP, S=10, R=20, A=rmail $u
Mprog,	P=/bin/sh,   F=lsDFMeuP,  S=10, R=20, A=sh -c $u

S10
# None needed.

S20
# None needed.

############################################################
#####
#####		Ethernet Mailer specification
#####
#####	Messages processed by this configuration are assumed to remain
#####	in the same domain.  This really has nothing particular to do
#####   with Ethernet - the name is historical.

Mether,	P=[TCP], E=\r\n, F=msDFMuCX, S=11, R=21, A=TCP $h
S11
R$*<@$+>$*		$@$1<@$2>$3			already ok
R$+			$@$1<@$w>			tack on our hostname

S21
# None needed.



##########################################################
#  General code to convert back to old style UUCP names
S5
R$+<@LOCAL>		$@ $D!$1			name@LOCAL => machine!name
R$+<@$-.LOCAL>		$@ $2!$1			u@h.LOCAL => h!u
R$+<@$+.uucp>		$@ $2!$1			u@h.uucp => h!u
R$+<@$*>		$@ $2!$1			u@h => h!u
# Route-addrs do not work here.  Punt til uucp-mail comes up with something.
R<@$+>$*		$@ @$1$2			just defocus and punt
R$*<$*>$*		$@ $1$2$3			Defocus strange stuff

#	UUCP Mailer specification

Muucp,	P=/usr/bin/uux, F=msDFMhuU, S=13, R=23,
	A=uux - -r $h!rmail ($u)

# Convert uucp sender (From) field
S13
R$+			$:$>5$1				convert to old style
R$=w!$+			$2				strip local name
R$+			$:$w!$1				stick on real host name

# Convert uucp recipient (To, Cc) fields
S23
R$+			$:$>5$1				convert to old style


#####		RULESET ZERO PREAMBLE

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

# arrange for local names to be fully qualified
#R$*<@$%y>$*		$1<@$2.LOCAL>$3			user@etherhost
# Substitute SUN-style %y (above) with hostlookup
R$*<@$+>$*		$: $1<@$(hostlookup $2 $)>$3	canonicalize
R$*<@$+.>$*		$#ether $@$2 $:$1<@$2>$3	remove trailing dot

# now delete redundant local info
R$*<$*$=w.LOCAL>$*	$1<$2>$4			thishost.LOCAL
R$*<@LOCAL>$*		$1<@$m>$2			host == domain gateway
R$*<$*$=w.uucp>$*	$1<$2>$4			thishost.uucp
R$*<$*$=w>$*		$1<$2>$4			thishost
R$*<$*.>$*		$1<$2>$3			drop trailing dot
R<@>:$*			$@$>30$1			retry after route strip
R$*<@>			$@$>30$1			strip null trash & retry


################################################
###  Machine dependent part of ruleset zero  ###
################################################

# resolve names we can handle locally
R<@$=V.uucp>:$+		$:$>9 $1			First clean up, then...
R<@$=V.uucp>:$+		$#uucp  $@$1 $:$2		@host.uucp:...
R$+<@$=V.uucp>		$#uucp  $@$2 $:$1		user@host.uucp

# optimize names of known ethernet hosts
#R$*<@$%y.LOCAL>$*	$#ether $@$2 $:$1<@$2>$3	user@host.here
# Substitute SUN-style %y (above) with hostlookup
R$*<@$+.LOCAL>$*	$: $1<@$(hostlookup $2 $).LOCAL>$3	canonicalize
R$*<@$+..LOCAL>$*	$#ether $@$2 $:$1<@$2>$3	remove trailing dot

# other non-local names will be kicked upstairs
R$+			$:$>9 $1			Clean up, keep <>
R$*<@$+>$*		$#$M    $@$R $:$1<@$2>$3	user@some.where
R$*@$*			$#$M    $@$R $:$1<@$2>		strangeness with @

# Local names with % are really not local!
R$+%$+			$@$>30$1@$2			turn % => @, retry

# everything else is a local name
R$+			$#local $:$1			local names
