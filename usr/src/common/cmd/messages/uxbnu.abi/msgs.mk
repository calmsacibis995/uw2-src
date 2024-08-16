#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)messages:common/cmd/messages/uxbnu.abi/msgs.mk	1.1"

include $(CMDRULES)


all	: msgs

install: all 
	cp msgs bnu.abi.str
	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 bnu.abi.str

lintit : 

clean :
	rm -f bnu.abi.str

clobber : clean

