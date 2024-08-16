#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)devintf:i386at/cmd/devintf/devintf.mk	1.3.10.3"
#ident "$Header: devintf.mk 2.0 91/07/11 $"

include $(CMDRULES)

SUBMAKES=devices groups mkdtab flpyconf

foo	: all

.DEFAULT	:	
		for submk in $(SUBMAKES) ; \
		do \
			cd $$submk ; \
			$(MAKE) -f $$submk.mk $(MAKEARGS) $@ ; \
			cd .. ; \
		done
