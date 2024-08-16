#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ktool:common/ktool/locklist/locklist.mk	1.2"

include $(CMDRULES)

all: locklist $(SGS)_locklist

$(SGS)_locklist: _locklist.o getpl.o
	$(CC) -o $(SGS)_locklist _locklist.o getpl.o \
		$(LDFLAGS) $(LDLIBS) $(SHLIBS)

_locklist.o: \
	$(INC)/stdio.h \
	$(INC)/malloc.h \
	$(INC)/string.h \
	$(INC)/stdlib.h

getpl.o:
# getpl.o: \
# 	$(TARGINC)/sys/ipl.h
	$(CC) $(CFLAGS) $(DEFLIST) -I$(TARGINC) -c getpl.c

clean:
	rm -f *.o

clobber: clean
	rm -f locklist $(SGS)_locklist

install: all
	-[ -d $(USRBIN) ] || mkdir -p $(USRBIN)
	$(INS) -f $(USRBIN) -m $(BINMODE) -u $(OWN) -g $(GRP) locklist
	-[ -d $(USRLIB) ] || mkdir -p $(USRLIB)
	$(INS) -f $(USRLIB) -m $(BINMODE) -u $(OWN) -g $(GRP) $(SGS)_locklist

lintit:
	$(LINT) $(LINTFLAGS) _locklist.c getpl.c
