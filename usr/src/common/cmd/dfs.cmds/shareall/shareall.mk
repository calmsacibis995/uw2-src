#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)dfs.cmds:shareall/shareall.mk	1.3.5.3"
#ident "$Header: shareall.mk 1.2 91/04/05 $"

include $(CMDRULES)

INSDIR = $(USRSBIN)
OWN = bin
GRP = bin
FRC =

all: shareall

shareall: shareall.sh 
	cp shareall.sh shareall
	chmod 555 shareall

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) shareall

clean:

clobber: clean
	rm -f shareall
FRC:
