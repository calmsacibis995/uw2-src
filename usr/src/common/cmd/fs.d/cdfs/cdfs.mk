#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1991, 1992  Intel Corporation
#	All Rights Reserved
#
#	INTEL CORPORATION CONFIDENTIAL INFORMATION
#
#	This software is supplied to USL under the terms of a license 
#	agreement with Intel Corporation and may not be copied nor 
#	disclosed except in accordance with the terms of that agreement.

#ident	"@(#)cdfs.cmds:common/cmd/fs.d/cdfs/cdfs.mk	1.2"
#ident	"$Header: $"

include $(CMDRULES)

all:
	@for i in *; do \
	    if [ -d $$i ]; then \
			echo $$i; \
			cd $$i; \
			$(MAKE) -f $$i.mk $(MAKEARGS) $@; \
			cd .. ; \
	    fi; \
	done

install:	all
	@for i in *; do \
	    if [ -d $$i ]; then \
			echo $$i; \
			cd $$i; \
			$(MAKE) -f $$i.mk $(MAKEARGS) $@; \
			cd .. ; \
	    fi; \
	done

headinstall:
	@for i in *; do \
	    if [ -d $$i ]; then \
			echo $$i; \
			cd $$i; \
			$(MAKE) -f $$i.mk $(MAKEARGS) $@; \
			cd .. ; \
	    fi; \
	done

lintit:
	@for i in *; do \
	    if [ -d $$i ]; then \
			echo $$i; \
			cd $$i; \
			$(MAKE) -f $$i.mk $(MAKEARGS) $@; \
			cd .. ; \
	    fi; \
	done

clean:
	@for i in *; do \
	    if [ -d $$i ]; then \
			echo $$i; \
			cd $$i; \
			$(MAKE) -f $$i.mk $(MAKEARGS) $@; \
			cd .. ; \
	    fi; \
	done

clobber:	clean
	@for i in *; do \
	    if [ -d $$i ]; then \
			echo $$i; \
			cd $$i; \
			$(MAKE) -f $$i.mk $(MAKEARGS) $@; \
			cd .. ; \
	    fi; \
	done
