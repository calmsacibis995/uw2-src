#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nucd:nucd.mk	1.16"
#ident  "$Header: /SRCS/esmp/usr/src/nw/cmd/nuc.d/nucd/nucd.mk,v 1.16.2.2 1995/01/27 17:00:30 mdash Exp $"

include $(CMDRULES)

LOCALDEF = -DIAPX386 -Di386 -DSVR4 -DSVr4 -DSVr4_1 -DN_PLAT_UNIX -D_KMEMUSER \
		-D_REENTRANT
LOCALINC = -I$(INC)/inc -I$(INC)/nw

#	Where MAINS are to be installed.
INSDIR = $(USRSBIN)

#
#	so this doesn't conform -- big deal..
#

NUCDOBJ		= nucd.o \
		  nucam.o \
		  nwlogin.o \
		  message.o \
		  unmount.o \
		  slogin.o \
		  sighup.o \
		  authen.o 

all: nucd

nucd: $(NUCDOBJ)
	$(CC) -o nucd $(NUCDOBJ) $(LDFLAGS) -lgen -lNwCal -lNwNcp -lNwClnt -lNwLoc -lnwutil -lthread -lsl -lnsl -lsocket -lthread -lnct -lgen

install: nucd
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	-rm -f $(INSDIR)/nucd
	$(INS) -f $(INSDIR) -m 04550 -u $(OWN) -g $(GRP) nucd
	
clean:
	rm -f $(NUCDOBJ)
	
clobber: clean
	rm -f nucd

lintit:
	$(LINT) $(LINTFLAGS) nucd.c
