#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:fs/ufs/ufs.mk	1.9"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	ufs.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = fs/ufs

UFS = ufs.cf/Driver.o
LFILE = $(LINTDIR)/ufs.ln

MODULES = \
	$(UFS)

FILES = ufs_vfsops.o

LFILES = ufs_vfsops.ln

CFILES = ufs_vfsops.c

SRCFILES = $(CFILES)

all:	$(MODULES)

install: all
	(cd ufs.cf; $(IDINSTALL) -R$(CONF) -M ufs)

$(UFS): $(FILES)
	$(LD) -r -o $(UFS) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(UFS)

clobber: clean 
	-$(IDINSTALL) -R$(CONF) -d -e ufs

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
	ufs_fs.h \
	ufs_inode.h \
	ufs_fsdir.h \
	ufs_quota.h

headinstall: $(sysfsHeaders)
	@-[ -d $(INC)/sys/fs ] || mkdir -p $(INC)/sys/fs
	@for f in $(sysfsHeaders); \
	 do \
	    $(INS) -f $(INC)/sys/fs -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
