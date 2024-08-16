#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mp.cmds:common/cmd/online/online.mk	1.2"
#ident	"$Header: $"

include $(CMDRULES)
OWN=root
GRP=sys
INSDIR=$(USRBIN)

all:	online.sh offline.sh
	cp online.sh  online
	cp offline.sh offline

install:	all
	-rm -f $(ETC)/online $(ETC)/offline
	-rm -f $(USRSBIN)/online $(USRBIN)/offline
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) online
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) offline
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) online
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) offline
	-$(SYMLINK) /sbin/online $(ETC)/online
	-$(SYMLINK) /sbin/offline $(ETC)/offline

clean:

clobber:	clean
	rm -f online offline

