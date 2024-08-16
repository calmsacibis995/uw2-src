#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)fdisk:i386at/cmd/fdisk/fdisk.mk	1.15"

include	$(CMDRULES)

MAINS	= fdisk fdisk.boot
MSGS	= fdisk.str
OBJS	= fdisk.o bootcod.o
BOBJS	= $(OBJS) fstub.o

ALL:		$(MAINS)

fdisk:  $(OBJS)
	$(CC) $(LDFLAGS) -o fdisk $(OBJS) -lgen -ladm

fdisk.boot:  $(BOBJS)
	$(CC) $(LDFLAGS) -o fdisk.boot $(BOBJS) -lgen

# uncomment the code below to make new bootcod.c

#bootcod.c: bin2c bootstrap.s
#	rm -f bootstrap.obj bootstrap bootstrap.o
#	as bootstrap.s
#	echo	'text=V0x600;' > piftmp
#	ld -o bootstrap.obj -Mpiftmp -dn bootstrap.o
#	./bin2c bootstrap.obj bootcod.c
 
#bin2c:	bin2c.o
#	cc -o bin2c bin2c.o -lelf

fdisk.c:            \
		 $(INC)/sys/types.h \
		 $(INC)/sys/vtoc.h \
		 $(INC)/sys/termios.h \
		 $(INC)/sys/fdisk.h \
		 $(INC)/curses.h \
		 $(INC)/term.h \
		 $(INC)/fcntl.h \
		 $(INC)/libgen.h

clean:
	rm -f $(BOBJS)

clobber:        clean
	rm -f $(MAINS)

all : ALL

install: ALL $(MSGS)
	$(INS) -f $(USRSBIN) fdisk
	$(INS) -f $(USRSBIN) fdisk.boot
	-[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 fdisk.str
