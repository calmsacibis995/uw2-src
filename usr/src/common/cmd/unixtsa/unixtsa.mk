#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)unixtsa:common/cmd/unixtsa/unixtsa.mk	1.3"
###
#
#  name		unixtsa.mk - build unix TSA
#		@(#)unixtsa:common/cmd/unixtsa/unixtsa.mk	1.3	4/1/94
#
###

TOP=.
include config.mk

SUBDIRS = \
	catalogs \
	tsalib \
	tsaunix \
	tsad

all install clean :
	Dir=`pwd`; $(MAKE) MTARG=$@ $(MAKEARGS) -f unixtsa.mk do_subdirs

 
force :
	touch */*.C
	$(MAKE) $(MAKEARGS) -f unixtsa.mk all


clobber :
	Dir=`pwd`; $(MAKE) MTARG=$@ $(MAKEARGS) -f unixtsa.mk do_subdirs
	rm -rf bin lib
