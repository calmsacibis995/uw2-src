#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)eucset:eucset.mk	1.3.2.2"
#ident  "$Header: eucset.mk 1.3 91/06/27 $"
#

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin
LDLIBS=	-lw
OFILES=	eucset.o

all:	eucset

eucset:	$(OFILES)
	$(CC) $(OFILES) -o $@ $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install:	eucset
	$(INS) -f $(INSDIR) -m 555 -u $(OWN) -g $(GRP) eucset

clean:
	rm -f $(OFILES)

clobber:	clean
	rm -f eucset

eucset.o:	$(INC)/sys/euc.h \
		$(INC)/getwidth.h \
		$(INC)/sys/eucioctl.h
