#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libnsl:common/lib/libnsl/netdir/netdir.mk	1.9.10.3"
#ident	"$Header: $"

# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# 		PROPRIETARY NOTICE (Combined)
# 
# This source code is unpublished proprietary information
# constituting, or derived under license from AT&T's UNIX(r) System V.
# In addition, portions of such source code were derived from Berkeley
# 4.3 BSD under license from the Regents of the University of
# California.
# 
# 
# 
# 		Copyright Notice 
# 
# Notice of copyright on this source code product does not indicate 
# publication.
# 
# 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
# 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
# 	          All rights reserved.
 
include $(LIBRULES)
include ../libnsl.rules

.SUFFIXES:	.c .o 

LOCALDEF=	$(NSL_LOCALDEF)
LIBOBJECTS=	../netdir.o  ../netdir_mt.o
OBJECTS=	netdir.o netdir_mt.o
SRCS=		netdir.c netdir_mt.c

all:		$(OBJECTS)
		if [ x$(CCSTYPE) != xCOFF ] ; \
		then \
			cp $(OBJECTS) ../ ; \
		fi

lintit: 
		$(LINT) $(LINTFLAGS) $(INCLIST) $(DEFLIST) $(SRCS)

clean:
		rm -f $(OBJECTS)

clobber:	clean
		rm -f $(LIBOBJECTS)

size:		all
		$(SIZE) $(LIBOBJECTS)

strip:		all
		$(STRIP) $(LIBOBJECTS)
