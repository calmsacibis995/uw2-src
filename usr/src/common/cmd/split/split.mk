#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#	Portions Copyright (c) 1988, Sun Microsystems, Inc.
#	All Rights Reserved.

#ident	"@(#)split:split.mk	1.3.7.3"
#ident "$Header: split.mk 1.2 91/03/19 $"

include $(CMDRULES)

#	Makefile for split

OWN = bin
GRP = bin

LOCAL_LDLIBS = $(LDLIBS) -lgen

all: split

split: split.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/statvfs.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/string.h \
	$(INC)/errno.h $(INC)/sys/errno.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LOCAL_LDLIBS) $(SHLIBS)

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) split

clean:
	rm -f split.o

clobber: clean
	rm -f split

lintit:
	$(LINT) $(LINTFLAGS) split.c

