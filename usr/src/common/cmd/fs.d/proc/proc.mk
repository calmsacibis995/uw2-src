#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proc.cmds:proc/proc.mk	1.13.4.1"
#ident "$Header: proc.mk 1.2 91/04/11 $"
#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

include $(CMDRULES)

INSDIR = $(ETC)/fs/proc
OWN = bin
GRP = bin

INCSYS = $(INC)
FRC =

all:	mount 

mount: mount.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

mount.o:	mount.c\
	$(INC)/stdio.h\
	$(INC)/signal.h\
	$(INC)/unistd.h\
	$(INC)/errno.h\
	$(INCSYS)/sys/mnttab.h\
	$(INCSYS)/sys/mount.h\
	$(INCSYS)/sys/types.h\
	$(FRC)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	[ -d $(USRLIB)/fs/proc ] || mkdir -p $(USRLIB)/fs/proc
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) mount
	$(INS) -f $(USRLIB)/fs/proc -m 0555 -u $(OWN) -g $(GRP) mount

clean:
	rm -f *.o

clobber:	clean
	rm -f mount
FRC:
