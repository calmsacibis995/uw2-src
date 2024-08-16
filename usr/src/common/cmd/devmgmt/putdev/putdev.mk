#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)devmgmt:common/cmd/devmgmt/putdev/putdev.mk	1.5.9.6"
#ident "$Header: putdev.mk 1.2 91/04/05 $"

include $(CMDRULES)

INSDIR=$(SBIN)
OWN=root
GRP=sys
HDRS=$(INC)/stdio.h $(INC)/string.h $(INC)/ctype.h $(INC)/stdlib.h \
	$(INC)/errno.h $(INC)/unistd.h $(INC)/mac.h \
	$(INC)/devmgmt.h
PROGRAM=putdev
SRC=main.c
OBJS=$(SRC:.c=.o)
LOCALINC=-I.
LDLIBS=-ladm
LINTFLAGS=$(DEFLIST)

all:	$(PROGRAM)

install:	all
		-rm -f $(INSDIR)/putdev
		$(INS) -f $(INSDIR) -m 555 -u $(OWN) -g $(GRP) $(PROGRAM) 

clobber: clean
		rm -f $(PROGRAM)

clean:
		rm -f $(OBJS)

lintit:
		for i in $(SRC); \
		do \
		    $(LINT) $(LINTFLAGS) $$i; \
		done

$(PROGRAM): $(OBJS)
		$(CC) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS) $(NOSHLIBS)

$(OBJS): $(HDRS)
