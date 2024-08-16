#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:fs/bfs/bfs.mk	1.4"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	bfs.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = fs/bfs

BFS = bfs.cf/Driver.o
LFILE = $(LINTDIR)/bfs.ln

MODULES = \
	$(BFS)

FILES = bfs_compact.o \
	 bfs_subr.o \
	 bfs_vfsops.o \
	 bfs_vnops.o

LFILES = bfs_compact.ln \
	 bfs_subr.ln \
	 bfs_vfsops.ln \
	 bfs_vnops.ln

CFILES = bfs_compact.c \
	 bfs_subr.c \
	 bfs_vfsops.c \
	 bfs_vnops.c

SRCFILES = $(CFILES)

all:	$(MODULES)

install: all
	(cd bfs.cf; $(IDINSTALL) -R$(CONF) -M bfs)

$(BFS):	$(FILES)
	$(LD) -r -o $(BFS) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(BFS)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e bfs

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES);	\
	do \
		echo $$i; \
	done

sysfsHeaders = \
	bfs.h 

headinstall: $(sysfsHeaders)
	@-[ -d $(INC)/sys/fs ] || mkdir -p $(INC)/sys/fs
	@for f in $(sysfsHeaders); \
	do \
		$(INS) -f $(INC)/sys/fs -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	done

include $(UTSDEPEND)

include $(MAKEFILE).dep
