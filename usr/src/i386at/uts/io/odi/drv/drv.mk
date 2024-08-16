#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/odi/drv/drv.mk	1.16"

#
# There are NO source files that exist within the ODI drivers.  The
# driver.o is already built prior to integration. Therefore, the
# only thing that this makefile should do is to cd to each subdirectory
# and idinstall what is there. However, the default targets (e.g., lintit,
# headinstall, etc) must still be in place (even though they do nothing)
# so that no errors occur when they are called from the parent directory
# makefile.
#

include $(UTSRULES)

MAKEFILE=	drv.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
SUBDIRS = \
	ES3210 \
	EWRK3 \
	IBM164 \
	IBMEST \
	IBMLST \
	ILANAT \
	INT32 \
	NCU9180 \
	NE1000 \
	NE1500T \
	NE2 \
	NE2_32 \
	NE2000 \
	NE2100 \
	NE3200 \
	NI9210 \
	NTR2000 \
	SMC8K \
	SMC8232 \
	SMC8332 \
	SMC8100 \
	SMC9K \
	TCM503 \
	TCM507 \
	TCM523 \
	TCM5X9 \
	TCTOKH \
	OCTOK162 \
	TOKENBM

all:

install: all
	@for d in $(SUBDIRS); do \
		(cd $$d/$$d.cf;  $(IDINSTALL) -R$(CONF) -M $$d ) ; \
	 done

clean:

clobber:	clean
	@for d in $(SUBDIRS); do \
		(cd $$d/$$d.cf; $(IDINSTALL) -R$(CONF) -e -d $$d) ; \
	done

$(LINTDIR):

lintit:

fnames:

headinstall:

depend:
