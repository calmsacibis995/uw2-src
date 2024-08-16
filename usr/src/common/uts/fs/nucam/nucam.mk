#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-nuc:fs/nucam/nucam.mk	1.11"
#ident 	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucam/nucam.mk,v 1.12 1994/11/30 00:26:13 ericw Exp $"

include $(UTSRULES)

DEVINC1 = -I.
LOCALDEF = 

KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = fs/nucam

NUCAM = nucam.cf/Driver.o       
LFILE = $(LINTDIR)/nucam.ln

MODULES	 = \
	 $(NUCAM)

FILES	= \
	amfs_childops.o \
	amfs_comops.o \
	amfs_nodeops.o \
	amfs_rootops.o \
	nucam_comops.o \
	nucam_vfsops.o \
	nucam_vnops.o

SRCFILES = \
	amfs_childops.c \
	amfs_comops.c \
	amfs_nodeops.c \
	amfs_rootops.c \
	nucam_comops.c \
	nucam_vfsops.c \
	nucam_vnops.c

LFILES = \
	amfs_childops.ln \
	amfs_comops.ln \
	amfs_nodeops.ln \
	amfs_rootops.ln \
	nucam_comops.ln \
	nucam_vfsops.ln \
	nucam_vnops.ln

all:	$(MODULES)

install: all
	(cd nucam.cf; $(IDINSTALL) -R$(CONF) -M nucam)

$(NUCAM): $(FILES)
	$(LD) -r -o $(NUCAM) $(FILES)

#
# Configuration Section
#

headinstall: \
	$(KBASE)/fs/nucam/amfs_node.h \
	$(KBASE)/fs/nucam/amfs_ops.h \
	$(KBASE)/fs/nucam/nucam_common.h \
	$(KBASE)/fs/nucam/nucam_nwc_proto.h \
	$(KBASE)/fs/nucam/nucam_glob.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/nucam/amfs_node.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/nucam/amfs_ops.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/nucam/nucam_common.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/nucam/nucam_nwc_proto.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/nucam/nucam_glob.h

clean:
	rm -f *.o $(LFILES) *.L $(NUCAM)

clobber:	clean
	$(IDINSTALL) -e -R$(CONF) -d nucam

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

