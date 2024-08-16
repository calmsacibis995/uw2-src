#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ypcmd:mknetid/mknetid.mk	1.1"
#ident  "$Header: $"

include $(CMDRULES)

LDLIBS = -lnsl
YP=$(VAR)/yp

SRCS = mknetid.c getname.c 
OBJS = $(SRCS:.c=.o)

MAKEFILE = mknetid.mk

all: mknetid

mknetid: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDLIBS)

lint:
	lint $(SRCS)

clean: 
	rm -f $(OBJS)

clobber: 
	rm -f mknetid $(OBJS)

install: mknetid
	install -f $(YP) mknetid 
 
mknetid.o: mknetid.c \
	$(INC)/stdio.h \
	$(INC)/pwd.h \
	$(INC)/limits.h \
	$(INC)/sys/param.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/key_prot.h

getname.o: getname.c \
	$(INC)/stdio.h \
	$(INC)/string.h
