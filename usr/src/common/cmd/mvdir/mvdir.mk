#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)mvdir:mvdir.mk	1.5.4.2"
#ident "$Header: mvdir.mk 1.2 91/04/16 $"

include $(CMDRULES)


OWN = root
GRP = bin

all: mvdir.sh
	cp mvdir.sh mvdir

install: all
	-rm -f $(ETC)/mvdir
	 $(INS) -f $(USRSBIN) -m 0544 -u $(OWN) -g $(GRP) mvdir
	-$(SYMLINK) /usr/sbin/mvdir $(ETC)/mvdir

clean:
	rm -f mvdir

clobber: clean

