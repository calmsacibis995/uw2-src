#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libnwutil:common/lib/libnutil/nwcm/libnwcm.mk	1.9"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"$Id: libnwcm.mk,v 1.28 1994/08/30 19:18:46 vtag Exp $"

include $(LIBRULES)

#include ../local.def

LOCALINC = \
		-I../nls/English \
		$(PICFLAG)

LINTSUBDIRS =	\
				tools

NWNETDIR = ./nwnet
NUCDIR = ./nuc
NPRINTDIR = ./nprinter
NETMGTDIR = ./netmgt

SCOMP		= ./tools/scomp ./tools/binbuild.o

SUBDIRS = $(LINTSUBDIRS)

.MUTEX::	$(SCOMP)

all install: $(SCOMP)
	[ -d $(ETC)/netware/conf ] || mkdir -p $(ETC)/netware/conf
	cd $(NWNETDIR) ; $(MAKE) -f *.mk $@ $(MAKEARGS)
	cd $(NETMGTDIR) ; $(MAKE) -f *.mk $@ $(MAKEARGS)
	cd $(NUCDIR) ; $(MAKE) -f *.mk $@ $(MAKEARGS)
	cd $(NPRINTDIR) ; $(MAKE) -f *.mk $@ $(MAKEARGS)

$(SCOMP):
	(cd tools ; make -f *.mk all)

clean: 
	(cd tools ; $(MAKE) -f *.mk clean)
	(cd $(NWNETDIR) ; $(MAKE) -f *.mk clean)
	(cd $(NETMGTDIR) ; $(MAKE) -f *.mk clean)
	(cd $(NUCDIR) ; $(MAKE) -f *.mk clean)
	(cd $(NPRINTDIR) ; $(MAKE) -f *.mk clean)

clobber:
	(cd tools ; $(MAKE) -f *.mk clobber)
	(cd $(NWNETDIR) ; $(MAKE) -f *.mk clobber)
	(cd $(NETMGTDIR) ; $(MAKE) -f *.mk clobber)
	(cd $(NUCDIR) ; $(MAKE) -f *.mk clobber)
	(cd $(NPRINTDIR) ; $(MAKE) -f *.mk clobber)


lintit:	
	-@(cd tools ; make -f *.mk lintit)
