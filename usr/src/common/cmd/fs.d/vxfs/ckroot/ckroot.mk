#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

# @(#)usr/src/common/cmd/fs.d/vxfs/ckroot/ckroot.mk	2.7 11 Oct 1994 11:47:35 - Copyright (c) 1994 VERITAS Software Corp.
#ident	"@(#)vxfs.cmds:common/cmd/fs.d/vxfs/ckroot/ckroot.mk	1.5.1.8"

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
LOCALDEF = 

INSDIR1 = $(USRLIB)/fs/vxfs
INSDIR2 = $(ETC)/fs/vxfs
OWN = bin
GRP = bin

PROBEFILE = ckroot.c
MAKEFILE = ckroot.mk
BINARIES = ckroot ckroot.dy

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
	$(CH)$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) ckroot
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) ckroot

clean:
	-rm -f ckroot.o

clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

binaries: $(BINARIES)

ckroot: ckroot.o
	$(CC) $(LDFLAGS) -o $@ ckroot.o $(LDLIBS) $(ROOTLIBS)

ckroot.dy: ckroot.o
	$(CC) $(LDFLAGS) -o $@ ckroot.o $(LDLIBS) $(SHLIBS)

ckroot.o: ckroot.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/errno.h \
	$(INC)/errno.h \
	$(INC)/fcntl.h \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/stdio.h \
	$(INC)/sys/statvfs.h \
	$(INC)/sys/mount.h \
	$(INC)/sys/uadmin.h \
	$(INC)/sys/vfstab.h \
	$(INC)/sys/fs/vx_ioctl.h
