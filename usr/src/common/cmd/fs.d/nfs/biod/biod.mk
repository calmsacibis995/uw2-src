#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nfs.cmds:biod/biod.mk	1.1"
#ident	"$Header: $"

include $(CMDRULES)

INCSYS = $(INC)
LOCALDEF = -Dnetselstrings
INSDIR = $(USRLIB)/nfs
OWN = bin
GRP = bin

all: biod

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) biod

clean:
	-rm -f biod.o

clobber: clean
	-rm -f biod

biod:	biod.o
	$(CC) -o $@ $(LDFLAGS) $@.o $(LDLIBS) $(SHLIBS)

biod.o:	biod.c $(INC)/stdio.h \
	$(INCSYS)/sys/types.h \
	$(INCSYS)/sys/file.h \
	$(INCSYS)/sys/ioctl.h
