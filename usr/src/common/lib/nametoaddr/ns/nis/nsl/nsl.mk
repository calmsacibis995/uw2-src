#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nametoaddr:common/lib/nametoaddr/ns/nis/nsl/nsl.mk	1.2"
#ident  "$Header: $"

#
#	STREAMware TCP
#	Copyright 1987, 1993 Lachman Technology, Inc.
#	All Rights Reserved.
#

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
include ../lnis.rules

OBJS= getrpcent.o

all:		$(OBJS)
		[ -d $(OBJDIR) ] || mkdir $(OBJDIR)
		cp $(OBJS) $(OBJDIR)

install:

clean:
		-rm -f *.o

clobber:	clean

#
# Header dependencies
#
getrpcent.o: getrpcent.c \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/rpcent.h \
	$(INC)/sys/syslog.h \
	$(NISINC)/nis.h \
	$(NISINC)/nis_mt.h \
	$(INC)/ns.h
