#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/odi/compat/compat.mk	1.1"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE= compat.mk
KBASE   = ../../..
LINTDIR = $(KBASE)/lintdir
DIR     = io/odi/compat

all:headinstall

install:all

clean:

clobber:clean

compatHeaders = \
	dlpi_lsl.h \
	msm.h \
	uwodi.h \
	ethtsm.h \
	sr_route.h

headinstall:$(compatHeaders)
	@for f in $(compatHeaders);     \
	do                              \
		$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN)    \
			-g $(GRP) $$f;  \
	done

include $(UTSDEPEND)
