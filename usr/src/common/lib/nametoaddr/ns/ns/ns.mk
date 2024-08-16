#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nametoaddr:common/lib/nametoaddr/ns/ns/ns.mk	1.1"
#ident  "$Header: $"


#
# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# 		PROPRIETARY NOTICE (Combined)
# 
# This source code is unpublished proprietary information
# constituting, or derived under license from AT&T's UNIX(r) System V.
# In addition, portions of such source code were derived from Berkeley
# 4.3 BSD under license from the Regents of the University of
# California.
# 
# 
# 
# 		Copyright Notice 
# 
# Notice of copyright on this source code product does not indicate 
# publication.
# 
# 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
# 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
#	(c) 1990,1991  UNIX System Laboratories, Inc.
# 	          All rights reserved.
#  

include $(LIBRULES)
include lns.rules

OBJS= nsfuncs.o  nsaction.o nsmt.o

all: nsobjs

nsobjs:		$(OBJS)
		[ -d $(OBJDIR) ] || mkdir $(OBJDIR)
		cp $(OBJS) $(OBJDIR)

install:	all

clean:
		-rm -f *.o

clobber:	clean

#
# Header dependencies
#

nsaction.o: nsaction.c \
	$(INC)/stdio.h \
	$(INC)/netdb.h \
	$(NSINC)/ns_mt.h 

nserrno.o: nserrno.c \
	$(INC)/stdio.h \
	$(INC)/netdb.h \
	$(NSINC)/ns_mt.h 

nsrtload.o: nsrtload.c \
	$(INC)/stdio.h \
	$(INC)/dlfcn.h \
	$(NSINC)/ns.h

nsmt.o: nsmt.c \
	$(INC)/stdlib.h \
	$(INC)/netdb.h \
	$(NSINC)/ns_mt.h
