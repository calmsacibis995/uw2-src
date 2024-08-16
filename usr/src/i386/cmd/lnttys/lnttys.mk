#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)lnttys:i386/cmd/lnttys/lnttys.mk	1.3.4.2"
#ident "$Header: lnttys.mk 1.2 91/04/18 $"

include $(CMDRULES)

OWN = root
GRP = root

all: lnttys.sh
	cp lnttys.sh lnttys

install: all
	$(INS) -f $(USRSBIN) -m 0744 -u $(OWN) -g $(GRP) lnttys

clean:

clobber: clean
	rm -f lnttys
