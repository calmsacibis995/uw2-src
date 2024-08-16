#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)logname:logname.mk	1.4.4.1"

include $(CMDRULES)

#	logname make file

OWN = bin
GRP = bin

SOURCES = logname.c

all: logname

logname: logname.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

logname.o: logname.c \
	$(INC)/stdio.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) logname

clean:
	rm -f logname.o

clobber: clean
	 rm -f logname

lintit:
	$(LINT) $(LINTFLAGS$(SOURCES)
