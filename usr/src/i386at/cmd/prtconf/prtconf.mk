#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)prtconf:i386at/cmd/prtconf/prtconf.mk	1.5.6.5"
#ident "$Header"

include $(CMDRULES)

INSDIR1	= $(USRSBIN)
DIRS	= $(INSDIR1)

O_CFILES = prtconf
OFILES = prtconf.o

OWN = bin
GRP = sys

all: $(O_CFILES)

$(O_CFILES): $(OFILES)
	$(CC) $(CFLAGS) $(LDFLAGS) $(DEFLIST) -o $@ $(OFILES) -ladm

clean:
	rm -f *.o

clobber: clean
	rm -f $(O_CFILES)

lintit:

size strip: all

$(DIRS):
	-mkdir -p $@

install: all $(DIRS)
	$(INS) -u $(OWN) -g $(GRP) -m 2555 -f $(INSDIR1) prtconf
