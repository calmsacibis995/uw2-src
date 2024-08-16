#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

# @(#)usr/src/i386/uts/fs/vxfs/vxfs.mk	2.29 19 Sep 1994 16:03:07 - Copyright (c) 1994 VERITAS Software Corp.
#ident	"@(#)kern-i386:fs/vxfs/vxfs.mk	1.21"

# Copyright (c) 1994 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
# UNPUBLISHED -- RIGHTS RESERVED UNDER THE COPYRIGHT
# LAWS OF THE UNITED STATES.  USE OF A COPYRIGHT NOTICE
# IS PRECAUTIONARY ONLY AND DOES NOT IMPLY PUBLICATION
# OR DISCLOSURE.
#
# THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
# TRADE SECRETS OF VERITAS SOFTWARE.  USE, DISCLOSURE,
# OR REPRODUCTION IS PROHIBITED WITHOUT THE PRIOR
# EXPRESS WRITTEN PERMISSION OF VERITAS SOFTWARE.
#
#	       RESTRICTED RIGHTS LEGEND
# USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
# SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
# (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
# COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
#	       VERITAS SOFTWARE
# 4800 GREAT AMERICA PARKWAY, SANTA CLARA, CA 95054

include $(UTSRULES)

KBASE    = ../..
VXBASE	 = ../..
#LOCALDEF = -DTED_
LOCALINC = -I $(VXBASE)

VXFS = vxfs.cf/Driver.o
MODULES = $(VXFS)

LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/vxfs.ln

PROBEFILE = vx_acl.c
MAKEFILE = vxfs.mk
BINARIES = $(VXFS)

TED_CFILE = vx_ted.c
TED_OBJ   = vx_ted.o
TED_LN	  = vx_ted.ln

TED_PRINT_CFILE	= vx_print.c
TED_PRINT_OBJ	= vx_print.o
TED_PRINT_LN	= vx_print.ln

CFILES = \
	vx_acl.c  \
	vx_alloc.c  \
	vx_attr.c  \
	vx_bio.c  \
	vx_bitmaps.c  \
	vx_bmap.c  \
	vx_dio.c  \
	vx_dira.c  \
	vx_dirl.c  \
	vx_dirop.c  \
	vx_dirsort.c \
	vx_full.c  \
	vx_getpage.c  \
	vx_ialloc.c  \
	vx_iflush.c  \
	vx_inode.c  \
	vx_itrunc.c  \
	vx_kernrdwri.c  \
	vx_lct.c  \
	vx_lite.c  \
	vx_log.c  \
	vx_map.c  \
	vx_message.c  \
	vx_mount.c  \
	vx_olt.c  \
	vx_oltmount.c  \
	$(TED_PRINT_CFILE)  \
	vx_putpage.c  \
	vx_quota.c  \
	vx_rdwri.c  \
	vx_reorg.c  \
	vx_resize.c  \
	vx_snap.c  \
	$(TED_CFILE)  \
	vx_tran.c  \
	vx_upgrade.c  \
	vx_vfsops.c  \
	vx_vnops.c 

LFILES = \
	vx_acl.ln  \
	vx_alloc.ln  \
	vx_attr.ln  \
	vx_bio.ln  \
	vx_bitmaps.ln  \
	vx_bmap.ln  \
	vx_dio.ln  \
	vx_dira.ln  \
	vx_dirl.ln  \
	vx_dirop.ln  \
	vx_dirsort.ln \
	vx_full.ln  \
	vx_getpage.ln  \
	vx_ialloc.ln  \
	vx_iflush.ln  \
	vx_inode.ln  \
	vx_itrunc.ln  \
	vx_kernrdwri.ln  \
	vx_lct.ln  \
	vx_lite.ln  \
	vx_log.ln  \
	vx_map.ln  \
	vx_message.ln  \
	vx_mount.ln  \
	vx_olt.ln  \
	vx_oltmount.ln  \
	$(TED_PRINT_LN)  \
	vx_putpage.ln  \
	vx_quota.ln  \
	vx_rdwri.ln  \
	vx_reorg.ln  \
	vx_resize.ln  \
	vx_snap.ln  \
	$(TED_LN) \
	vx_tran.ln  \
	vx_upgrade.ln  \
	vx_vfsops.ln  \
	vx_vnops.ln 

COMMON_OBJS = \
	vx_acl.o  \
	vx_alloc.o  \
	vx_attr.o  \
	vx_bio.o  \
	vx_bitmaps.o  \
	vx_bmap.o  \
	vx_dira.o  \
	vx_dirl.o  \
	vx_dirop.o  \
	vx_getpage.o  \
	vx_ialloc.o  \
	vx_iflush.o  \
	vx_inode.o  \
	vx_itrunc.o  \
	vx_kernrdwri.o  \
	vx_lct.o  \
	vx_log.o  \
	vx_map.o  \
	vx_message.o  \
	vx_mount.o  \
	vx_olt.o  \
	vx_oltmount.o  \
	$(TED_PRINT_OBJ) \
	vx_putpage.o  \
	vx_quota.o  \
	vx_rdwri.o  \
	$(TED_OBJ) \
	vx_tran.o  \
	vx_upgrade.o  \
	vx_vfsops.o  \
	vx_vnops.o

FULL_ONLY_OBJS = \
	vx_dio.o  \
	vx_dirsort.o  \
	vx_full.o  \
	vx_reorg.o  \
	vx_resize.o  \
	vx_snap.o

LITE_ONLY_OBJS = vx_lite.o

VJFS_OBJS = $(COMMON_OBJS) $(LITE_ONLY_OBJS)

VXFS_OBJS = $(COMMON_OBJS) $(FULL_ONLY_OBJS)

all:
	@if [ -f $(PROBEFILE) ]; then \
		find $(BINARIES) \( ! -type f -o -links +1 \) \
		    -exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
		$(MAKE) -f $(MAKEFILE) binaries $(MAKEARGS) "KBASE=$(KBASE)" \
			"LOCALDEF=$(LOCALDEF)" ;\
	else \
		for fl in $(BINARIES); do \
			if [ ! -r $$fl ]; then \
				echo "ERROR: $$fl is missing" 1>&2 ;\
				false ;\
				break ;\
			fi \
		done \
	fi

clean:
	-rm -f $(VXFS_OBJS) $(VJFS_OBJS) $(LFILES) *.L 

clobber: clean
	-$(IDINSTALL) -e -R$(CONF) -d vxfs
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" Driver_vx.o ;\
		rm -f $(BINARIES) Driver_vx.o ;\
	fi

$(LINTDIR):
	[ -d $@ ] || mkdir -p $@

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(CFILES);	\
	do \
		echo $$i; \
	done

#
# Configuration Section
#

install: all
	(cd vxfs.cf; $(IDINSTALL) -R$(CONF) -M vxfs)

headinstall: \
	$(VXBASE)/fs/vxfs/vx_bitmaps.h \
	$(VXBASE)/fs/vxfs/vx_buf.h \
	$(VXBASE)/fs/vxfs/vx_dir.h \
	$(VXBASE)/fs/vxfs/vx_ext.h \
	$(VXBASE)/fs/vxfs/vx_fs.h \
	$(VXBASE)/fs/vxfs/vx_info.h \
	$(VXBASE)/fs/vxfs/vx_inode.h \
	$(VXBASE)/fs/vxfs/vx_ioctl.h \
	$(VXBASE)/fs/vxfs/vx_log.h \
	$(VXBASE)/fs/vxfs/vx_param.h \
	$(VXBASE)/fs/vxfs/vx_proto.h \
	$(VXBASE)/fs/vxfs/vx_tran.h
	[ -d $(INC)/sys/fs ] || mkdir -p $(INC)/sys/fs
	[ -d t ] || mkdir t
	for fl in vx_bitmaps.h vx_buf.h vx_dir.h vx_ext.h vx_fs.h vx_inode.h \
		    vx_ioctl.h vx_log.h vx_param.h vx_proto.h vx_tran.h; do \
		rm -f t/$$fl ;\
		grep -v TED_ $(VXBASE)/fs/vxfs/$$fl > t/$$fl ;\
		$(INS) -f $(INC)/sys/fs -m 644 -u $(OWN) -g $(GRP) t/$$fl ;\
	done
	rm -fr t

binaries: $(BINARIES) Driver_vx.o

$(BINARIES): $(VJFS_OBJS)
	$(LD) -r -o $@ $(VJFS_OBJS)

Driver_vx.o: $(VXFS_OBJS)
	$(LD) -r -o $@ $(VXFS_OBJS)


include $(UTSDEPEND)

include $(MAKEFILE).dep
