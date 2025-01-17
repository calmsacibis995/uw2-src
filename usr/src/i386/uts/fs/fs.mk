#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386:fs/fs.mk	1.64"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	fs.mk
KBASE = ..
LINTDIR = $(KBASE)/lintdir
DIR = fs

FS = fs.cf/Driver.o
DOW = dow.cf/Driver.o
LFILE = $(LINTDIR)/fs.ln

MODULES = \
	$(DOW) \
	$(FS)

FILES = \
	bio.o \
	fsflush.o \
	bufsubr.o \
	dnlc.o \
	fbio.o \
	fio.o	\
	flock.o	\
	vncalls.o \
	fs_subr.o \
	lookup.o \
	pathname.o \
	vfs.o \
	vnode.o \
	pipe.o

DOWFILES = \
	dow_create.o \
	dow_io.o \
	dow_order.o \
	dowlink_util.o \
	dow_cancel.o \
	dow_flush.o \
	dow_leaf.o \
	dow_prune.o \
	dow_check.o \
	dow_handle.o \
	dow_util.o

CFILES = \
	bio.c \
	fsflush.c \
	bufsubr.c \
	dnlc.c \
	fbio.c \
	fio.c	\
	flock.c	\
	vncalls.c \
	fs_subr.c \
	lookup.c \
	pathname.c \
	vfs.c \
	dow_create.c \
	dow_io.c \
	dow_order.c \
	dowlink_util.c \
	dow_cancel.c \
	dow_flush.c \
	dow_leaf.c \
	dow_prune.c \
	dow_check.c \
	dow_handle.c \
	dow_util.c \
	vnode.c \
	pipe.c

SRCFILES = $(CFILES)

LFILES = \
	bio.ln \
	fsflush.ln \
	bufsubr.ln \
	dnlc.ln \
	fbio.ln \
	fio.ln \
	flock.ln \
	fs_subr.ln \
	lookup.ln \
	vncalls.ln \
	pathname.ln \
	vfs.ln \
	dow_create.ln \
	dow_io.ln \
	dow_order.ln \
	dowlink_util.ln \
	dow_cancel.ln \
	dow_flush.ln \
	dow_leaf.ln \
	dow_prune.ln \
	dow_check.ln \
	dow_handle.ln \
	dow_util.ln \
	vnode.ln \
	pipe.ln

SUBDIRS = \
	specfs ufs sfs bfs fifofs namefs profs s5fs procfs fdfs xnamfs nfs \
	cdfs xxfs memfs vxfs dosfs nucam nucfs

all:	local FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk all"; \
		 $(MAKE) -f $$d.mk all $(MAKEARGS)); \
	 done

local:	$(MODULES)

$(FS): $(FILES)
	$(LD) -r -o $(FS) $(FILES)

$(DOW): $(DOWFILES)
	$(LD) -r -o $(DOW) $(DOWFILES)

install: localinstall FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk install"; \
		 $(MAKE) -f $$d.mk install $(MAKEARGS)); \
	 done

localinstall: local FRC
	cd fs.cf; $(IDINSTALL) -R$(CONF) -M fs
	cd dow.cf; $(IDINSTALL) -R$(CONF) -M dow

clean:	localclean
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clean"; \
		 $(MAKE) -f $$d.mk clean $(MAKEARGS)); \
	 done

localclean:
	-rm -f *.o $(LFILES) *.L $(MODULES)

localclobber:	localclean
	-$(IDINSTALL) -R$(CONF) -d -e fs
	-$(IDINSTALL) -R$(CONF) -d -e dow

clobber:	localclobber
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clobber"; \
		 $(MAKE) -f $$d.mk clobber $(MAKEARGS)); \
	 done

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE) FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk lintit"; \
		 $(MAKE) -f $$d.mk lintit $(MAKEARGS)); \
	 done

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for d in $(SUBDIRS); do \
	    (cd $$d; \
		$(MAKE) -f $$d.mk fnames $(MAKEARGS) | \
		$(SED) -e "s;^;$$d/;"); \
	done
	@for f in $(SRCFILES); do \
		echo $$f; \
	done

headinstall: localhead FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk headinstall"; \
		 $(MAKE) -f $$d.mk headinstall $(MAKEARGS)); \
	 done

sysHeaders = \
	buf.h \
	buf_f.h \
	dir.h \
	dirent.h \
	dnlc.h \
	fblk.h \
	fbuf.h \
	fcntl.h \
	file.h \
	filsys.h \
	filio.h \
	flock.h \
	fs_subr.h \
	fstyp.h \
	fsid.h \
	ino.h \
	ioccom.h \
	mkfs.h \
	mnttab.h \
	mode.h \
	mount.h \
	pathname.h \
	select.h \
	stat.h \
	mntent.h \
	statfs.h \
	statvfs.h \
	ustat.h \
	vfs.h \
	vfstab.h \
	vnode.h

localhead: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

FRC:

include $(UTSDEPEND)

depend::
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk depend";\
		 touch $$d.mk.dep;\
		 $(MAKE) -f $$d.mk depend $(MAKEARGS));\
	done

include $(MAKEFILE).dep
