#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)wsinit:wsinit.mk	1.8"

#
# 	wsinit.mk:
# 	makefile for the wsinit command
#

include	$(CMDRULES)

LOCALDEF = -D_LTYPES

MSGS	= wsinit.str

all:	wsinit wsinit.dy

install:	all $(MSGS)
		cp ./wstations.sh workstations
		$(INS) -f $(SBIN) -m 0554 -u 0 -g 3 ./wsinit
		$(INS) -f $(SBIN) -m 0554 -u 0 -g 3 ./wsinit.dy
		$(INS) -f $(ETC)/default -m 0644 -u 0 -g 3 ./workstations
		-rm -rf workstations
		-[ -d $(USRLIB)/locale/C/MSGFILES ] || \
			mkdir -p $(USRLIB)/locale/C/MSGFILES
		$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 wsinit.str

clean:
	rm -f *.o

clobber: clean
	rm -f workstations
	rm -f wsinit

wsinit.o:	wsinit.c \
	$(INC)/sys/genvid.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/mkdev.h \
	$(INC)/sys/stat.h

wsinit:	wsinit.o
	$(CC) -o wsinit wsinit.o $(LDFLAGS) $(ROOTLIBS)

wsinit.dy:	wsinit.o
	$(CC) -o wsinit.dy wsinit.o $(LDFLAGS)
