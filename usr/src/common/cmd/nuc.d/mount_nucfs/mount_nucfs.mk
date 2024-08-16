#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mount_nucfs:mount_nucfs.mk	1.5"
#ident  "$Header: /SRCS/esmp/usr/src/nw/cmd/nuc.d/mount_nucfs/mount_nucfs.mk,v 1.7 1994/12/01 19:51:29 ericw Exp $"

include $(CMDRULES)


#	Where MAINS are to be installed.
INSDIR = $(USRLIB)/fs/nucfs

LOCALDEF = -DIAPX386 -Di386 -DSVR4 -DSVr4 -DSVr4_1 \
	-DOLDSTYLEBIND -DN_PLAT_UNIX -DN_INC_NO_OLD_MACROS -DN_USE_CRT 

all: mount

mount: mount.o mount_nucfs.o
	$(CC) -o mount mount.o mount_nucfs.o $(LDFLAGS) -lnsl -lsocket -lnwutil -lthread -lNwClnt -lNwNcp -lNwCal -lNwLoc -lgen

install: mount
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	rm -f $(INSDIR)/mount
	$(INS) -f $(INSDIR) -m 04550 -u $(OWN) -g $(GRP) mount
	
clean:
	rm -f mount.o
	rm -f mount_nucfs.o
	
clobber: clean
	rm -f mount

lintit:
	$(LINT) $(LINTFLAGS) mount.c
	$(LINT) $(LINTFLAGS) mount_nucfs.c
