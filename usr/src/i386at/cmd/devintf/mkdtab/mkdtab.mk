#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)devintf:i386at/cmd/devintf/mkdtab/mkdtab.mk	1.6"
#ident "$Header: mkdtab.mk 1.2 91/07/24 $"

include $(CMDRULES)

INSDIR = $(USRSADM)/sysadm/bin
ARCH=AT386
BUS=AT386

LOCALDEF=-D$(ARCH) -D$(BUS)
LDLIBS = -ladm -lgen

all: mkdtab

mkdtab: mkdtab.c\
	$(INC)/stdio.h\
	$(INC)/stdlib.h\
	$(INC)/string.h\
	$(INC)/fcntl.h\
	$(INC)/unistd.h\
	$(INC)/devmgmt.h\
	$(INC)/sys/mkdev.h\
	$(INC)/sys/cram.h\
	$(INC)/sys/param.h\
	$(INC)/sys/stat.h\
	$(INC)/sys/vtoc.h\
	$(INC)/sys/vfstab.h\
	mkdtab.h
	$(CC) $(DEFLIST) $(CFLAGS) mkdtab.c -o mkdtab $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -u bin -g bin -m 0555 mkdtab

clean:
	rm -f *.o

clobber: clean
	rm -f mkdtab

lintit:

