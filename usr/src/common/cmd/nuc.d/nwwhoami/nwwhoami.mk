#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nwwhoami:nwwhoami.mk	1.6"
include $(CMDRULES)

#	Where MAINS are to be installed.
INSDIR = $(USRBIN)

LOCALDEF = -DIAPX386 -Di386 -DSVR4 -DSVr4 -DSVr4_1 -DOLDSTYLEBIND -DN_PLAT_UNIX -DN_INC_NO_OLD_MACROS -DN_USE_CRT

DEV1 = -I../../../head

all: nwwhoami

nwwhoami: nwwhoami.o
	$(CC) -o nwwhoami nwwhoami.o $(LDFLAGS) \
         -lNwCal -lNwNcp -lNwClnt -lNwLoc -lgen -lnsl $(TOOLS)/usr/lib/novell.so \
		 -lnwutil -lthread -lnct

install: nwwhoami
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	rm -f $(INSDIR)/nwwhoami
	$(INS) -f $(INSDIR) -m 04550 -u $(OWN) -g $(GRP) nwwhoami
	
clean:
	rm -f nwwhoami.o
	
clobber: clean
	rm -f nwwhoami

lintit:
	$(LINT) $(LINTFLAGS) nwwhoami.c

