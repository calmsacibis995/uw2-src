#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nwchecksums:nwchecksums.mk	1.6"

include $(CMDRULES)

#	Where MAINS are to be installed.
INSDIR = $(USRSBIN)

LOCALDEF = -DIAPX386 -Di386 -DSVR4 -DSVr4 -DSVr4_1 -DOLDSTYLEBIND -DN_PLAT_UNIX -DN_INC_NO_OLD_MACROS -DN_USE_CRT


all: nwchecksums

nwchecksums: nwchecksums.o
	$(CC) -o nwchecksums nwchecksums.o $(LDFLAGS) -lNwClnt -lNwCal -lNwNcp -lNwLoc -lnwutil -lthread -lnsl -lgen -lnct

install: nwchecksums
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	rm -f $(INSDIR)/nwchecksums
	$(INS) -f $(INSDIR) -m 0550 -u $(OWN) -g $(GRP) nwchecksums
	
clean:
	rm -f nwchecksums.o
	
clobber: clean
	rm -f nwchecksums

lintit:
	$(LINT) $(LINTFLAGS) nwchecksums.c
