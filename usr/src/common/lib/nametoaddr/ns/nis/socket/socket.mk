#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nametoaddr:common/lib/nametoaddr/ns/nis/socket/socket.mk	1.2"
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
include ../lnis.rules

OBJS= ether_addr.o getnetent.o getprotoent.o \
	getservent.o inet_sethost.o

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
ether_addr.o: ether_addr.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/socket.h \
	$(INC)/net/if.h \
	$(INC)/netinet/in.h \
	$(INC)/net/if_arp.h \
	$(INC)/netinet/if_ether.h \
	$(NISINC)/nis.h \
	$(INC)/ns.h

getnetent.o: getnetent.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/socket.h \
	$(INC)/netdb.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/netinet/in.h \
	$(NISINC)/nis.h \
	$(NISINC)/nis_mt.h \
	$(INC)/ns.h

getprotoent.o: getprotoent.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/socket.h \
	$(INC)/netdb.h \
	$(INC)/ctype.h \
	$(NISINC)/nis.h \
	$(NISINC)/nis_mt.h \
	$(INC)/ns.h

getservent.o: getservent.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/socket.h \
	$(INC)/netdb.h \
	$(INC)/ctype.h \
	$(NISINC)/nis.h \
	$(NISINC)/nis_mt.h \
	$(INC)/ns.h

inet_sethost.o: inet_sethost.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/socket.h \
	$(INC)/netdb.h \
	$(INC)/ctype.h \
	$(INC)/netinet/in.h \
	$(NISINC)/nis.h \
	$(NISINC)/nis_mt.h \
	$(INC)/ns.h
