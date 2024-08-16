#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:fs/cdfs/cdfs.mk	1.7"

include $(UTSRULES)

MAKEFILE=	cdfs.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = fs/cdfs

CDFS = cdfs.cf/Driver.o
LFILE = $(LINTDIR)/cdfs.ln

MODULES = \
	$(CDFS)

FILES = \
	cdfs_bmap.o \
	cdfs_data.o \
	cdfs_dir.o \
	cdfs_inode.o \
	cdfs_ioctl.o \
	cdfs_subr.o \
	cdfs_susp.o \
	cdfs_vfsops.o \
	cdfs_vnops.o

LFILES = \
	cdfs_bmap.ln \
	cdfs_dir.ln \
	cdfs_inode.ln \
	cdfs_ioctl.ln \
	cdfs_subr.ln \
	cdfs_susp.ln \
	cdfs_vfsops.ln \
	cdfs_vnops.ln

CFILES = \
	cdfs_bmap.c \
	cdfs_data.c \
	cdfs_dir.c \
	cdfs_inode.c \
	cdfs_ioctl.c \
	cdfs_subr.c \
	cdfs_susp.c \
	cdfs_vfsops.c \
	cdfs_vnops.c	

SRCFILES = $(CFILES)

all: $(MODULES)

install: all
	(cd cdfs.cf; $(IDINSTALL) -R$(CONF) -M cdfs)

$(CDFS): $(FILES)
	$(LD) -r -o $(CDFS) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(CDFS)


clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e cdfs

$(LINTDIR):
	-mkdir -p $@

lintit: $(LFILE)

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

sysHeaders = \
	cdrom.h

sysfsHeaders = \
	cdfs_inode.h \
	cdfs_susp.h \
	iso9660.h \
	cdfs_ioctl.h \
	cdfs_fs.h

headinstall: $(sysHeaders) $(sysfsHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@-[ -d $(INC)/sys/fs ] || mkdir -p $(INC)/sys/fs
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done
	@for f in $(sysfsHeaders); \
	 do \
	    $(INS) -f $(INC)/sys/fs -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
