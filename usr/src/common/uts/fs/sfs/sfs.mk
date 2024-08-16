#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:fs/sfs/sfs.mk	1.11"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	sfs.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = fs/sfs

SFS = sfs.cf/Driver.o
LFILE = $(LINTDIR)/sfs.ln

MODULES = \
	$(SFS)

FILES = sfs_dir.o \
	sfs_alloc.o \
	sfs_subr.o \
	sfs_bmap.o \
	sfs_inode.o \
	sfs_vfsops.o \
	sfs_vnops.o \
	sfs_tables.o \
	sfs_data.o \
	sfs_qcalls.o \
	sfs_qlims.o \
	sfs_quota.o \
	sfs_dow.o

LFILES = sfs_dir.ln \
	 sfs_alloc.ln \
	 sfs_subr.ln \
	 sfs_bmap.ln \
	 sfs_inode.ln \
	 sfs_vfsops.ln \
	 sfs_vnops.ln \
	 sfs_tables.ln \
	 sfs_data.ln \
	 sfs_qcalls.ln \
	 sfs_qlims.ln \
	 sfs_quota.ln \
	 sfs_dow.ln

CFILES = sfs_dir.c \
	 sfs_alloc.c \
	 sfs_subr.c \
	 sfs_bmap.c \
	 sfs_inode.c \
	 sfs_vfsops.c \
	 sfs_vnops.c \
	 sfs_tables.c \
	 sfs_data.c \
	 sfs_qcalls.c \
	 sfs_qlims.c \
	 sfs_quota.c \
	 sfs_dow.c

SRCFILES = $(CFILES)

all:	$(MODULES)

install: all
	(cd sfs.cf; $(IDINSTALL) -R$(CONF) -M sfs)

$(SFS):	$(FILES)
	$(LD) -r -o $(SFS) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(SFS)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e sfs

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
	sfs_fs.h \
	sfs_fsdir.h \
	sfs_inode.h \
	sfs_quota.h \
	sfs_tables.h

headinstall: $(sysfsHeaders)
	@-[ -d $(INC)/sys/fs ] || mkdir -p $(INC)/sys/fs
	@for f in $(sysfsHeaders); \
	 do \
	    $(INS) -f $(INC)/sys/fs -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
