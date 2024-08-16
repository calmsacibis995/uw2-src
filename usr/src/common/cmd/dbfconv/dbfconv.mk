#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)dbfconv:dbfconv.mk	1.6.3.1"
#ident "$Header: dbfconv.mk 1.3 91/04/08 $"

include $(CMDRULES)
INSDIR = $(USRLIB)/saf
OWN = root
GRP = sys

SOURCE = dbfconv.c nlsstr.c
OBJECT = dbfconv.o nlsstr.o
FRC =

all: dbfconv

dbfconv: $(OBJECT)
	$(CC) -o $@ $(OBJECT) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

$(INSDIR):
	mkdir $@

install: all $(INSDIR)
	$(INS) -f $(INSDIR) -m 0755 -u $(OWN) -g $(GRP) dbfconv

clean:
	rm -f $(OBJECT)

clobber: clean
	rm -f dbfconv

FRC:


dbfconv.o: $(INC)/sys/param.h \
	$(INC)/sys/fs/s5dir.h \
	local.h \
	$(INC)/stdio.h
