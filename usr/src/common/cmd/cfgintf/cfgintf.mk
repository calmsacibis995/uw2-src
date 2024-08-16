#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cfgintf:common/cmd/cfgintf/cfgintf.mk	1.1.5.3"
#ident "$Header: cfgintf.mk 2.0 91/07/11 $"

include $(CMDRULES)

# SUBMAKES=system summary logins
SUBMAKES=system summary di

foo		: all

.DEFAULT	:	
		for submk in $(SUBMAKES) ; \
		do \
		    if [ -d $$submk ] ; \
		    then \
		    	cd $$submk ; \
		    	$(MAKE) -f $$submk.mk $@ $(MAKEARGS); \
		    	cd .. ; \
		    fi \
		done
