#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)true:common/cmd/true/true.mk	1.4.7.2"
#ident "$Header: true.mk 1.4 91/06/12 $"

include $(CMDRULES)

OWN = root
GRP = sys

all: install

install:
	cp true.sh true
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) true

clean:
	rm -f true

clobber: clean
	rm -f true
