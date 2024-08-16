#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:fs/nfs/nfs.mk	1.12"
#ident	"$Header: $"

include $(UTSRULES)

# NFSESV is used to compile with the extended ESV protocol. However,
# this is not supported, and is not even gauranteed to compile.
# NFSDL is used to compile some psuedo support for remote devices.
# (only used for diskless workstations which are not supported).
#LOCALDEF = -DSYSV -DNFSESV -DNFSDL
LOCALDEF = -DSYSV

MAKEFILE=	nfs.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = fs/nfs

NFS = nfs.cf/Driver.o
NFSS = nfss.cf/Driver.o
MODSTUB = nfs.cf/Modstub.o
LFILE = $(LINTDIR)/nfs.ln

MODULES = \
	$(NFS) \
	$(NFSS) \
	$(MODSTUB)

FILES = nfs_majmin.o \
	 nfs_attr.o \
	 nfs_cnvt.o \
	 nfs_common.o \
	 nfs_export.o \
	 nfs_remote.o \
	 nfs_srv.o \
	 nfs_nfsd.o \
	 nfs_dispatch.o \
	 nfs_rcall.o \
	 nfs_clnt.o \
	 nfs_rnode.o \
	 nfs_io.o \
	 nfs_vfsops.o \
	 nfs_vnops.o \
	 nfs_init.o \
	 nfs_lk.o \
	 nfs_xdr.o \
	 nfs_sys.o \
	 nfs_static.o

LFILES = nfs_majmin.ln \
	 nfs_attr.ln \
	 nfs_cnvt.ln \
	 nfs_common.ln \
	 nfs_export.ln \
	 nfs_remote.ln \
	 nfs_srv.ln \
	 nfs_nfsd.ln \
	 nfs_dispatch.ln \
	 nfs_rcall.ln \
	 nfs_clnt.ln \
	 nfs_rnode.ln \
	 nfs_io.ln \
	 nfs_vfsops.ln \
	 nfs_vnops.ln \
	 nfs_init.ln \
	 nfs_lk.ln \
	 nfs_xdr.ln \
	 nfs_sys.ln \
	 nfs_static.ln

CFILES = nfs_majmin.c \
	 nfs_attr.c \
	 nfs_cnvt.c \
	 nfs_common.c \
	 nfs_export.c \
	 nfs_remote.c \
	 nfs_srv.c \
	 nfs_nfsd.c \
	 nfs_dispatch.c \
	 nfs_rcall.c \
	 nfs_clnt.c \
	 nfs_rnode.c \
	 nfs_io.c \
	 nfs_vfsops.c \
	 nfs_vnops.c \
	 nfs_init.c \
	 nfs_lk.c \
	 nfs_xdr.c \
	 nfs_sys.c \
	 nfs_static.c

SRCFILES = $(CFILES)

NFSFILES = nfs_majmin.o \
	 nfs_attr.o \
	 nfs_cnvt.o \
	 nfs_common.o \
	 nfs_export.o \
	 nfs_remote.o \
	 nfs_srv.o \
	 nfs_nfsd.o \
	 nfs_dispatch.o \
	 nfs_rcall.o \
	 nfs_clnt.o \
	 nfs_rnode.o \
	 nfs_io.o \
	 nfs_vfsops.o \
	 nfs_vnops.o \
	 nfs_init.o \
	 nfs_lk.o \
	 nfs_xdr.o \
	 nfs_sys.o

NFSLFILES = nfs_majmin.ln \
	 nfs_attr.ln \
	 nfs_cnvt.ln \
	 nfs_common.ln \
	 nfs_export.ln \
	 nfs_remote.ln \
	 nfs_srv.ln \
	 nfs_nfsd.ln \
	 nfs_dispatch.ln \
	 nfs_rcall.ln \
	 nfs_clnt.ln \
	 nfs_rnode.ln \
	 nfs_io.ln \
	 nfs_vfsops.ln \
	 nfs_vnops.ln \
	 nfs_init.ln \
	 nfs_lk.ln \
	 nfs_xdr.ln \
	 nfs_sys.ln

NFSSRCFILES = nfs_majmin.c \
	 nfs_attr.c \
	 nfs_cnvt.c \
	 nfs_common.c \
	 nfs_export.c \
	 nfs_remote.c \
	 nfs_srv.c \
	 nfs_nfsd.c \
	 nfs_dispatch.c \
	 nfs_rcall.c \
	 nfs_clnt.c \
	 nfs_rnode.c \
	 nfs_io.c \
	 nfs_vfsops.c \
	 nfs_vnops.c \
	 nfs_init.c \
	 nfs_lk.c \
	 nfs_xdr.c \
	 nfs_sys.c

all:	$(MODULES)

install: all
	(cd nfs.cf; $(IDINSTALL) -R$(CONF) -M nfs)
	(cd nfss.cf; $(IDINSTALL) -R$(CONF) -M nfss)

$(NFS):	$(NFSFILES)
	$(LD) -r -o $(NFS) $(NFSFILES)

$(NFSS): nfs_static.o
	$(LD) -r -o $(NFSS) nfs_static.o

$(MODSTUB): nfs_stub.o
	$(LD) -r -o $@ nfs_stub.o 

clean:
	-rm -f *.o $(LFILES) *.L $(NFS) $(NFSS) $(MODSTUB)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e nfs
	-$(IDINSTALL) -R$(CONF) -d -e nfss

$(LINTDIR):
	-mkaux -p $@

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES);  \
	do \
		echo $$i; \
	done

nfsHeaders = \
	export.h \
	mount.h \
	nfs.h \
	nfs_clnt.h \
	nfssys.h \
	rnode.h

headinstall: $(nfsHeaders)
	@-[ -d $(INC)/nfs ] || mkdir -p $(INC)/nfs
	@for f in $(nfsHeaders); \
	 do \
		$(INS) -f $(INC)/nfs -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
