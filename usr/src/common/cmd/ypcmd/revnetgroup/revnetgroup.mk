#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ypcmd:revnetgroup/revnetgroup.mk	1.1"
#ident  "$Header: $"

include $(CMDRULES)

HDRS = getgroup.h table.h util.h
SRCS = revnetgroup.c getgroup.c table.c util.c
OBJS = revnetgroup.o getgroup.o table.o util.o
YP=$(VAR)/yp

all:	revnetgroup

revnetgroup: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o revnetgroup

lint:
	lint $(SRCS)

clean: 
	rm -f $(OBJS)

clobber: clean 
	rm -f revnetgroup


install: all
	install -f $(YP) revnetgroup 

revnetgroup.o: revnetgroup.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/pwd.h \
	$(INC)/rpcsvc/ypclnt.h \
	util.h \
	table.h \
	getgroup.h

getgroup.o: getgroup.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	table.h \
	util.h \
	getgroup.h

table.o: table.c \
	$(INC)/ctype.h \
	util.h \
	table.h

util.o: util.c \
	$(INC)/stdio.h \
	util.h

