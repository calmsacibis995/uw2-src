#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)getacl:getacl.mk	1.3.3.2"
#ident  "$Header: getacl.mk 1.3 91/06/27 $"

include $(CMDRULES)

#	Makefile for getacl 

all: getacl

getacl: getacl.o 
	$(CC) -o getacl getacl.o  $(LDFLAGS) $(LDLIBS) $(NOSHLIBS)

getacl.o: getacl.c \
	$(INC)/stdio.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/pwd.h \
	$(INC)/grp.h \
	$(INC)/acl.h $(INC)/sys/acl.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/priv.h $(INC)/sys/privilege.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h

install: all
	-rm -f $(ROOT)/bin/getacl
	$(INS) -f $(USRBIN) getacl

clean:
	rm -f getacl.o

clobber: clean
	rm -f getacl

lintit:
	$(LINT) $(LINTFLAGS) getacl.c

#	These targets are useful but optional

partslist:
	@echo getacl.mk getacl.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo getacl | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit getacl.mk $(LOCALINCS) getacl.c -o getacl.o getacl
