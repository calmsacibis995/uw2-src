#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)mv:mv.mk	1.16.14.3"

include $(CMDRULES)

#	Makefile for mv/cp/ln

OWN = bin
GRP = bin

LIST = lp

MAINS = mv ln cp mv.dy ln.dy cp.dy

all: 	mv mv.dy
	-rm -f cp ln cp.dy ln.dy
	@/bin/ln mv cp
	@/bin/ln mv ln
	@/bin/ln mv.dy cp.dy
	@/bin/ln mv.dy ln.dy

mv: mv.o
	$(CC) -o mv mv.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

mv.dy: mv.o
	$(CC) -o mv.dy mv.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

mv.o: mv.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/fcntl.h \
	$(INC)/utime.h \
	$(INC)/signal.h \
	$(INC)/errno.h \
	$(INC)/sys/param.h \
	$(INC)/dirent.h \
	$(INC)/stdlib.h \
	$(INC)/acl.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/string.h \
	$(INC)/priv.h	\
	$(INC)/limits.h	\
	$(INC)/unistd.h		\
	$(INC)/nl_types.h		\
	$(INC)/langinfo.h

install: all
	$(CH)-rm -f $(USRBIN)/ln $(USRBIN)/cp
	$(CH)-rm -f $(USRBIN)/ln.dy $(USRBIN)/cp.dy
	$(CH)-rm -f $(SBIN)/ln $(SBIN)/cp
	$(CH)-rm -f $(SBIN)/ln.dy $(SBIN)/cp.dy
	 $(INS) -o -m 0555 -u $(OWN) -g $(GRP) -f $(SBIN) mv
	 $(INS) -o -m 0555 -u $(OWN) -g $(GRP) -f $(USRBIN) mv.dy
	-/bin/mv $(USRBIN)/mv.dy $(USRBIN)/mv
	$(CH)/bin/ln $(USRBIN)/mv $(USRBIN)/ln
	$(CH)/bin/ln $(USRBIN)/mv $(USRBIN)/cp
	$(CH)/bin/ln $(SBIN)/mv $(SBIN)/ln
	$(CH)/bin/ln $(SBIN)/mv $(SBIN)/cp

clean:
	rm -f mv.o

clobber: clean
	rm -f $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) mv.c

#	These targets are useful but optional

partslist:
	@echo mv.mk mv.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo $(MAINS) | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit mv.mk $(LOCALINCS) mv.c -o mv.o $(MAINS)

listing:
	pr mv.mk $(SOURCE) | $(LIST)

listmk: 
	pr mv.mk | $(LIST)
