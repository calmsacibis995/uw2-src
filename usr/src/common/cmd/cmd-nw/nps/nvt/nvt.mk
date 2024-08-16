#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/nvt/nvt.mk	1.3"
#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nps/nvt/nvt.mk,v 1.1 1994/01/28 17:30:35 vtag Exp $"

include $(CMDRULES)

TOP = ../..

include $(TOP)/local.def

all install clean clobber: 
	(cd nvtd ; \
	$(MAKE) -f nvtd.mk $@ $(MAKEARGS))

lintit: 
	-@(cd nvtd ; \
	$(MAKE) -f nvtd.mk $@ )
