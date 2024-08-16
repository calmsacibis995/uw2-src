#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)rtld:rtld.mk	1.9"
#
# makefile for rtld
#
#

include $(LIBRULES)

OWN=bin
GRP=bin
SGSBASE=../../../../cmd/sgs
INS=$(SGSBASE)/sgs.install

CKMACH="\
	if [ ! -d \"$(CPU)\" ] ; \
	then \
		echo \"The machine dependent directory, \$(CPU) is missing\";exit 2;\
	fi "
#
all:	
	@eval $(CKMACH) 2>/dev/null ;\
	cd $(CPU); $(MAKE) SGSBASE=$(SGSBASE) all

install:	all
	echo nothing to do

clean:
	#
	# remove intermediate files except object modules and temp library
	@eval $(CKMACH) 2>/dev/null ;\
	cd $(CPU); $(MAKE) clean

clobber:
	#
	# remove intermediate files
	@eval $(CKMACH) 2>/dev/null ;\
	cd $(CPU); $(MAKE) clobber

lintit:
	#
	@eval $(CKMACH) 2>/dev/null ;\
	cd $(CPU) ;\
	$(MAKE) SGSBASE=$(SGSBASE) lintit

backup:	backup.sh
	/bin/sh backup.sh
