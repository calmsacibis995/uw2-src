#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)false:common/cmd/false/false.mk	1.4.7.3"
#ident "$Header: false.mk 1.2 91/08/05 $"

include $(CMDRULES)

OWN = bin
GRP = bin

all:
	cp false.sh false
	chmod 0755 false

install:	all
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) false

clean:

clobber:	clean
	rm -f false
