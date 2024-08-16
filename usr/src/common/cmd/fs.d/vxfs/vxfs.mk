#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

# @(#)usr/src/common/cmd/fs.d/vxfs/vxfs.mk	2.7 21 Sep 1994 12:00:26 - Copyright (c) 1994 VERITAS Software Corp.
#ident	"@(#)vxfs.cmds:common/cmd/fs.d/vxfs/vxfs.mk	1.3.1.6"

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

# Make all the subdirectories and install

include $(CMDRULES)
DIRS= 	lib \
	ckroot \
	df \
	ff \
	fsadm \
	fscat \
	fsck \
	fsdb \
	fstyp \
	getext \
	labelit \
	mkfs \
	mount \
	ncheck \
	setext \
	volcopy \
	vxdump \
	vxrestore \
	vxupgrade

#
#  This is to build all the vxfs commands
#
.DEFAULT:
	for i in $(DIRS);\
	do\
	    if [ -d $$i -a -f $$i/$$i.mk ]; \
		then \
		cd  $$i;\
		$(MAKE) -f $$i.mk $(MAKEARGS) $@ ; \
		cd .. ; \
	    fi;\
	done

