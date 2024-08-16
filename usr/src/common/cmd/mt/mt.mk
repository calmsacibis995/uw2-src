#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mt:mt.mk	1.1"

include $(CMDRULES)

OWN = bin
GRP = bin

MAINS = mt

SOURCES =  mt.sh

all:		$(MAINS)

mt:		mt.sh
	cp mt.sh mt

clobber: 
	rm -f $(MAINS)

install: all
	 $(INS) -f  $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) $(MAINS)

clean:

lintit:

