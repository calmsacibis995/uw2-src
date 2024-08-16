#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ypcmd:msgs/msgs.mk	1.2"
#ident  "$Header: $"

include $(CMDRULES)

MSGS =  makedbm.str mkalias.str mknetid.str passwdd.str \
	revnetgroup.str stdethers.str stdhosts.str udpublickey.str \
	ypalias.str ypbind.str ypcat.str ypmatch.str yppasswd.str \
	yppoll.str yppush.str ypserv.str ypset.str ypshad2pwd.str \
	ypupdated.str ypwhich.str ypxfr.str ypinit.str

all: $(MSGS)

install: all
	-[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	@for m in $(MSGS);\
	do\
		$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 $$m;\
	done

clean:

lintit:

clobber:

