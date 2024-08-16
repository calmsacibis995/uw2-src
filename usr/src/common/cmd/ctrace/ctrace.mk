#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ctrace:ctrace.mk	1.17.1.8"

include $(CMDRULES)

ENVPARMS= \
	CMDRULES="$(CMDRULES)" 

all:
	cd $(CPU) ; $(MAKE) all $(ENVPARMS)

install:
	cd $(CPU) ; $(MAKE) install $(ENVPARMS)


lintit:
	cd $(CPU) ; $(MAKE) lintit $(ENVPARMS)

clean:
	cd $(CPU) ; $(MAKE) clean

clobber:
	cd $(CPU) ; $(MAKE) clobber
