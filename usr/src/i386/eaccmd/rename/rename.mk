#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)eac:i386/eaccmd/rename/rename.mk	1.1.3.2"
#ident  "$Header: rename.mk 1.3 91/08/12 $"

include $(CMDRULES)

#
# authsh's helpers	-- Accounts administration shell helper programs
#
#	@(#) Makefile 22.2 90/01/11 
#
# Copyright (c) 1989, 1990, The Santa Cruz Operation, Inc.,
# and SecureWare, Inc.  All rights reserved.
#
# This Module contains Proprietary Information of the Santa Cruz
# Operation, Inc., and SecureWare, Inc., and should be treated
# as Confidential. 
#
# The code marked with symbols from the list below, is owned
# by The Santa Cruz Operation Inc., and represents SCO value
# added portions of source code requiring special arrangements
# with SCO for inclusion in any product.
#  Symbol:		 Market Module:

OWN = bin
GRP = bin

INSDIR = $(USR)/eac

RENAME_SOURCES = rename.c
RENAME_OBJECTS = rename.o

#
all: rename

rename: $(RENAME_OBJECTS)
	$(CC) -o $@ $(RENAME_OBJECTS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

rename.o: rename.c \
	$(INC)/unistd.h \
	$(INC)/stdio.h \
	$(INC)/errno.h

install: $(INSDIR) all
	-rm -f $(ETC)/rename
	 $(INS) -f $(INSDIR) -m 0711 -u $(OWN) -g $(GRP) rename
	-ln $(INSDIR)/rename $(ETC)/rename

$(INSDIR):
	-mkdir -p $@
	$(CH)chmod 0755 $@
	$(CH)chgrp $(GRP) $@
	$(CH)chown $(OWN) $@

clean:
	-rm -f $(RENAME_OBJECTS)

clobber: clean
	-rm -f rename

lintit:
	$(LINT) $(LINTFLAGS) $(RENAME_SOURCES)
