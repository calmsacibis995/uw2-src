#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwcm/nwcm.mk	1.9"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwcm/nwcm.mk,v 1.12 1994/05/11 16:40:34 mark Exp $"

include $(CMDRULES)

TOP = ..

include ../local.def

LINTFLAGS = $(WORKINC) $(LOCALDEF) $(GLOBALINC) $(LOCALINC) -c

LOCALINC = -DNWCM_FRIEND

LDLIBS = $(LIBUTIL) -lm

OWN = root
GRP = sys

all: nwcm

nwcm: nwcm.o 
	$(CC) -o nwcm nwcm.o  $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

nwcm.o:	nwcm.c \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/ctype.h  \
	$(INC)/string.h

install: all
	 $(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) nwcm

clean:
	rm -f nwcm.o
	rm -f nwcm.ln

clobber: clean
	rm -f nwcm lint.out

lintit:
	-@rm -f lint.out ; \
	echo '## lint output for nwcm.c ##' >>lint.out ; \
	$(LINT) $(LINTFLAGS) nwcm.c $(LINTLIBS) >>lint.out
