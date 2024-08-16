#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)kill:kill.mk	1.12.7.2"
#ident  "$Header: kill.mk 1.3 91/06/27 $"

include $(CMDRULES)


OWN = bin
GRP = bin

all: kill

kill:
	echo \#\!/sbin/sh > kill
	echo /sbin/sh -c \"kill $$\*\" >> kill

clean:

clobber: clean
	rm -f kill

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) kill
