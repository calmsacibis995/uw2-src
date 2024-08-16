#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ktool:common/ktool/idtools/idtarg/idtarg.mk	1.17"
#ident	"$Header: $"

# Target-only ID/TP commands.
# These don't get cross-environment versions built.

include $(CMDRULES)

LDLIBS = -lgen

LOCALINC = -I..
DFLDIR = $(ROOT)/$(MACH)/etc/default

SHELLS = idreboot idrebuild idcpunix
CMDS =	idspace \
	idmknod \
	idmkinit \
	idmkenv \
	idmodreg \
	idkname \
	idmodload \
	idcheck \
	idresadd

INSDIR = $(CONF)/bin

CFILES = \
	idcheck.c \
	idmkenv.c \
	idmkinit.c \
	idmknod.c \
	idspace.c \
	idmodreg.c \
	idkname.c \
	idmodload.c \
	idresadd.c

all:	$(CMDS)

clean:
	-rm -f *.o

clobber: clean
	-rm -f $(CMDS)

install: all $(SHELLS)
	-[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	for f in $(CMDS) $(SHELLS); \
	do \
		$(INS) -f $(INSDIR) -m $(BINMODE) -u $(OWN) -g $(GRP) $$f; \
	done
	-[ -d $(DFLDIR) ] || mkdir -p $(DFLDIR)
	rm -f $(DFLDIR)/idtools
	cp idtools.dfl $(DFLDIR)/idtools

lintit:
	for f in $(CFILES); \
	do \
		$(LINT) $(LINTFLAGS) $$f; \
	done

IDRESADD_OFILES = idresadd.o ../getinst.o ../getmajors.o ../entry.o ../util.o
idresadd: $(IDRESADD_OFILES)
	$(CC) -o idresadd $(IDRESADD_OFILES) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

idresadd.o: \
	../inst.h \
	../devconf.h

IDCHECK_OFILES = idcheck.o ../getinst.o ../getmajors.o ../entry.o ../util.o
idcheck: $(IDCHECK_OFILES)
	$(CC) -o idcheck $(IDCHECK_OFILES) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

idcheck.o: \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	../inst.h \
	../defines.h \
	../mdep.h

IDMKNOD_OFILES = idmknod.o ../getinst.o ../getmajors.o ../devconf.o ../entry.o \
		../mdep.o ../util.o ../setlevel.o
idmknod: $(IDMKNOD_OFILES)
	$(CC) -o idmknod $(IDMKNOD_OFILES) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

idmknod.o: \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/errno.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/types.h \
	$(INC)/sys/dirent.h \
	$(INC)/sys/stat.h \
	../inst.h \
	../devconf.h

IDMKINIT_OFILES = idmkinit.o ../setlevel.o
idmkinit: $(IDMKINIT_OFILES)
	$(CC) -o idmkinit $(IDMKINIT_OFILES) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

idmkinit.o: \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	$(INC)/sys/dirent.h \
	../inst.h

IDSPACE_OFILES = idspace.o
idspace: $(IDSPACE_OFILES)
	$(CC) -o idspace $(IDSPACE_OFILES) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

idspace.o: \
	$(INC)/stdio.h \
	$(INC)/errno.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/types.h \
	$(INC)/sys/statvfs.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/mnttab.h \
	$(INC)/ustat.h \
	../inst.h

IDMKENV_OFILES = idmkenv.o ../setlevel.o
idmkenv: $(IDMKENV_OFILES)
	$(CC) -o idmkenv $(IDMKENV_OFILES) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

idmkenv.o: \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/fcntl.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/dirent.h \
	$(INC)/pwd.h \
	$(INC)/grp.h \
	$(INC)/varargs.h \
	../inst.h

IDMODREG_OFILES = idmodreg.o
idmodreg: $(IDMODREG_OFILES)
	$(CC) -o idmodreg $(IDMODREG_OFILES) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

idmodreg.o: \
	../inst.h \
	../defines.h \
	$(INC)/ctype.h \
	$(INC)/stdlib.h \
	$(INC)/fcntl.h \
	$(INC)/errno.h \
	$(INC)/varargs.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/param.h \
	$(INC)/unistd.h

IDMODLOAD_OFILES = idmodload.o ../getinst.o ../getmajors.o ../util.o ../entry.o
idmodload: $(IDMODLOAD_OFILES)
	$(CC) -o idmodload $(IDMODLOAD_OFILES) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

idmodload.o: \
	../inst.h \
	../defines.h \
	../devconf.h \
	$(INC)/sys/param.h

IDKNAME_OFILES = idkname.o bus.o
idkname: $(IDKNAME_OFILES)
	$(CC) -o idkname $(IDKNAME_OFILES) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

idkname.o: \
	../inst.h \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/errno.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/ksym.h \
	$(INC)/sys/elf.h
