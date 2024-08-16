#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

# @(#)usr/src/common/cmd/fs.d/vxfs/vxupgrade/vxupgrade.mk	2.9 11 Oct 1994 11:47:50 - Copyright (c) 1994 VERITAS Software Corp.
#ident	"@(#)vxfs.cmds:common/cmd/fs.d/vxfs/vxupgrade/vxupgrade.mk	1.7"

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

include $(CMDRULES)

LDLIBS = -lgen
LIBDIR	= ../lib
LOCALINC=-I$(LIBDIR) 
INSDIR1 = $(USRLIB)/fs/vxfs
INSDIR2 = $(USRSBIN)
OWN = bin
GRP = bin

PROBEFILE = vxupgrade.c
MAKEFILE = vxupgrade.mk
BINARIES = vxupgrade

all:
	@if [ -f $(PROBEFILE) ]; then \
		find $(BINARIES) \( ! -type f -o -links +1 \) \
		    -exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
		$(MAKE) -f $(MAKEFILE) binaries $(MAKEARGS) ;\
	else \
		for fl in $(BINARIES); do \
			if [ ! -f $$fl ]; then \
				echo "ERROR: $$fl is missing" 1>&2 ;\
				false ;\
				break ;\
			fi \
		done \
	fi

install: all
	[ -d $(INSDIR1) ] || mkdir -p $(INSDIR1)
	[ -d $(INSDIR2) ] || mkdir -p $(INSDIR2)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) vxupgrade
	$(CH)-rm -f $(INSDIR2)/vxupgrade
	$(CH)ln $(INSDIR1)/vxupgrade $(INSDIR2)/vxupgrade

clean:
	-rm -f vxupgrade.o

clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

binaries: $(BINARIES)


vxupgrade: vxupgrade.o $(LIBDIR)/libvxfs.a
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBDIR)/libvxfs.a $(SHLIBS) $(LDLIBS)

vxupgrade.o: vxupgrade.c \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/errno.h \
	$(INC)/sys/param.h \
	$(INC)/time.h \
	$(INC)/stdlib.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/fs/vx_inode.h \
	$(INC)/sys/fs/vx_ioctl.h \
	$(INC)/sys/fs/vx_fs.h \
	$(INC)/sys/fs/vx_param.h \
	$(INC)/sys/fs/vx_dir.h \
	$(INC)/sys/fs/vx_ext.h \
	$(INC)/sys/mnttab.h \
	$(INC)/sys/vfstab.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/statvfs.h \
	$(INC)/string.h \
	$(INC)/unistd.h \
	$(LIBDIR)/fs_subr.h
