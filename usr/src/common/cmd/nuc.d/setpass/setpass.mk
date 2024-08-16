#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)setpass:setpass.mk	1.4"
include $(CMDRULES)

INSDIR = $(USRBIN)

LOCALDEF = -DIAPX386 -Di386 -DSVR4 -DSVr4 -DSVr4_1 -DOLDSTYLEBIND -DN_PLAT_UNIX -DN_INC_NO_OLD_MACROS -DN_USE_CRT

DEV1 = -I../../../head

all: setpass

setpass: setpass.o
	$(CC) -o setpass setpass.o $(LDFLAGS) \
         -lNwCal -lNwNcp -lNwClnt -lNwLoc -lnct \
         $(TOOLS)/usr/lib/novell.so -lnwutil -lthread -lgen -lnsl
#         -lNwCal -lNwNcp -lNwAtb -lNwClnt -lnct /usr/lib/novell.so -lgen -lnsl

install: setpass
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	rm -f $(INSDIR)/setpass
	$(INS) -f $(INSDIR) -m 04550 -u $(OWN) -g $(GRP) setpass
	
clean:
	rm -f setpass.o
	
clobber: clean
	rm -f setpass

lintit:
	$(LINT) $(LINTFLAGS) setpass.c
