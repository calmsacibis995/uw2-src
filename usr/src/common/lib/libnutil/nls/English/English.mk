#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libnwutil:common/lib/libnutil/nls/English/English.mk	1.9"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"$Header: /SRCS/esmp/usr/src/nw/lib/libnutil/nls/English/English.mk,v 1.16 1994/08/30 22:34:28 mark Exp $"

include $(LIBRULES)

install clean clobber lintit:
	cd nwnet ; \
	$(MAKE) -f *.mk $@
	cd netmgt ; \
	$(MAKE) -f *.mk $@
	cd nuc ; \
	$(MAKE) -f *.mk $@
	cd nprinter ; \
	$(MAKE) -f *.mk $@

all: install
