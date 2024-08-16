#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)setacl:setacl.mk	1.3.2.2"
#ident  "$Header: setacl.mk 1.3 91/07/01 $"

include $(CMDRULES)

#	Makefile for setacl 

OWN = bin
GRP = bin

all: setacl

setacl: setacl.o 
	$(CC) -o setacl setacl.o  $(LDFLAGS) $(LDLIBS) $(NOSHLIBS)

setacl.o: setacl.c \
	$(INC)/stdio.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/pwd.h \
	$(INC)/grp.h \
	$(INC)/acl.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/priv.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h

install: all
	-rm -f $(USRBIN)/setacl
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) setacl

clean:
	rm -f setacl.o

clobber: clean
	rm -f setacl

lintit:
	$(LINT) $(LINTFLAGS) setacl.c

#	These targets are useful but optional

partslist:
	@echo setacl.mk setacl.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo setacl | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit setacl.mk $(LOCALINCS) setacl.c -o setacl.o setacl
