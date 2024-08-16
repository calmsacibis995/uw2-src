#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:fs/memfs/memfs.mk	1.6"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	memfs.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = fs/memfs

MEMFS = memfs.cf/Driver.o
LFILE = $(LINTDIR)/memfs.ln

MODULES = \
	$(MEMFS)

FILES = memfs_dir.o \
	memfs_mnode.o \
	memfs_subr.o \
	memfs_vfsops.o \
	memfs_vnops.o

LFILES = memfs_dir.ln \
	memfs_mnode.ln \
	memfs_subr.ln \
	memfs_vfsops.ln \
	memfs_vnops.ln

CFILES = memfs_dir.c \
	memfs_mnode.c \
	memfs_subr.c \
	memfs_vfsops.c \
	memfs_vnops.c

SRCFILES = $(CFILES)

all:	$(MODULES)

install: all
	cd memfs.cf; $(IDINSTALL) -R$(CONF) -M memfs

$(MEMFS):	$(FILES)
	$(LD) -r -o $(MEMFS) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(MEMFS)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e memfs

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
	@for i in $(SRCFILES); \
	do \
		echo $$i; \
	done

sysHeaders = \
	memfs.h \
	memfs_mnode.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys/fs ] || mkdir -p $(INC)/sys/fs
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys/fs -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
