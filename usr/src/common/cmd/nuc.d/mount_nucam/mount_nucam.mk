#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mount_nucam:mount_nucam.mk	1.8"
#ident  "$Header: /SRCS/esmp/usr/src/nw/cmd/nuc.d/mount_nucam/mount_nucam.mk,v 1.7 1994/12/01 19:51:24 ericw Exp $"

include $(CMDRULES)


#	Where MAINS are to be installed.
INSDIR = $(USRLIB)/fs/nucam

LOCALDEF = -DIAPX386 -Di386 -DSVR4 -DSVr4 -DSVr4_1

all: mount

mount: mount.o mount_nucam.o
	$(CC) -o mount mount.o mount_nucam.o $(LDFLAGS) -lNwCal -lNwNcp -lNwClnt -lNwLoc -lnwutil -lthread -lnsl -lsocket -lgen

install: mount
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	rm -f $(INSDIR)/mount
	$(INS) -f $(INSDIR) -m 04550 -u $(OWN) -g $(GRP) mount
	
clean:
	rm -f mount.o
	rm -f mount_nucam.o
	
clobber: clean
	rm -f mount

lintit:
	$(LINT) $(LINTFLAGS) mount.c
	$(LINT) $(LINTFLAGS) mount_nucam.c
