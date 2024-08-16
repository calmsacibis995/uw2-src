#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)messages:i386/cmd/messages/messages.mk	1.3.3.11"

include $(CMDRULES)

foo : all

.DEFAULT : 
	for submk in `ls` ; \
	do \
		[ -f $$submk/msgs.mk ] || continue ; \
	 	cd $$submk ; \
	 	$(MAKE) -f msgs.mk $@ ; \
	 	cd .. ; \
	done

