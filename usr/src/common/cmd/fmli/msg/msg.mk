#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#
#ident	"@(#)fmli:msg/msg.mk	1.1"
#

include $(CMDRULES)

MSGS = fmli.str

fmli.str:	uxfmli.src
	-cp uxfmli.src $(MSGS)

all:	$(MSGS)

install: all
	-[ -d $(USRLIB)/locale/C/MSGFILES ] || \
			mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 fmli.str

clean: 
	-rm -f $(MSGS)

clobber:	clean
