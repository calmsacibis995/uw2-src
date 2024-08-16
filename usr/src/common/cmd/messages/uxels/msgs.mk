#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)messages:common/cmd/messages/uxels/msgs.mk	1.1"

include $(CMDRULES)


all	: msgs

install	: all 
	cp msgs els.str
	if [ ! -d $(USRLIB)/locale/C/MSGFILES ]; \
		then mkdir -p $(USRLIB)/locale/C/MSGFILES; fi; \
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 0644 els.str

lintit	: 

clean	:
	rm -f els.str

clobber	: clean
