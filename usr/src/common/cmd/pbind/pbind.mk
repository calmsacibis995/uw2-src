#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mp.cmds:common/cmd/pbind/pbind.mk	1.1"

include $(CMDRULES)

OWN = root
GRP = sys
CMDS= pbind 
SOURCES= pbind.c 

all: $(CMDS)

pbind: pbind.o
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

install: all
	-rm -f $(ETC)/pbind
	-rm -f $(USRSBIN)/pbind
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) pbind
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) pbind
	-$(SYMLINK) /sbin/pbind $(ETC)/pbind

clean:
	rm -f *.o

clobber: clean
	rm -f $(CMDS)

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)
