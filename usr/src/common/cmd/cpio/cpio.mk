#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)cpio:common/cmd/cpio/cpio.mk	1.1.15.1"
#ident  "$Header: cpio.mk 1.3 91/06/25 $"

# Makefile for cpio

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN=bin
GRP=bin

CPIO_DEPEND =  cpio.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/errno.h \
	$(INC)/unistd.h \
	$(INC)/fcntl.h \
	$(INC)/memory.h \
	$(INC)/string.h \
	$(INC)/stdarg.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/mkdev.h \
	$(INC)/sys/secsys.h \
	$(INC)/sys/statvfs.h \
	$(INC)/sys/fs/vx_ioctl.h \
	$(INC)/utime.h \
	$(INC)/sys/time.h \
	$(INC)/pwd.h \
	$(INC)/grp.h \
	$(INC)/signal.h \
	$(INC)/ctype.h \
	$(INC)/archives.h \
	$(INC)/locale.h \
	cpio.h \
	$(INC)/priv.h \
	$(INC)/acl.h \
	$(INC)/mac.h \
	$(INC)/pfmt.h \
	$(INC)/sys/param.h \
	$(INC)/libgen.h \
	$(INC)/termios.h \
	$(INC)/sys/mman.h \
	decompress.h \
	$(INC)/dlfcn.h

DEFLIST = -D_REENTRANT
LDLIBS = -lgen -lgenIO

MAKEFILE = Makefile

MAINS = cpio cpio.dy cpio.boot

OBJECTS =  cpio.o cpiostat.o decompress.o
OBJECTS_BOOT = cpio.boot.o cpiostat.o decompress.o

SOURCES =  cpio.c

all:		$(MAINS)

cpio:		$(OBJECTS)	
	$(CC) -o cpio $(DEFLIST) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -lthread

cpio.dy:		$(OBJECTS)	
	$(CC) -o cpio.dy $(DEFLIST) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -lthread

cpio.boot:		$(OBJECTS_BOOT)	
	$(CC) -o cpio.boot $(OBJECTS_BOOT) $(LDFLAGS) $(LDLIBS)

cpio.o: $(CPIO_DEPEND) $(INC)/thread.h $(INC)/synch.h
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c $<

cpio.boot.o: $(CPIO_DEPEND)
	cp cpio.c $(@:.o=.c)
	$(CC) $(CFLAGS) $(INCLIST) -c $(@:.o=.c)
	rm $(@:.o=.c)

cpiostat.o: cpiostat.c \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	cpio.h

install: all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) cpio
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) cpio.dy
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) cpio.boot

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

size: all
	$(SIZE) $(MAINS)

strip: all
	$(STRIP) $(MAINS)

#	These targets are useful but optional

partslist:
	@echo $(MAKEFILE) $(SOURCES) $(LOCALINCS)  |  tr ' ' '\012'  |  sort

productdir:
	@echo $(INSDIR) | tr ' ' '\012' | sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
	sed 's;^;$(INSDIR)/;'
