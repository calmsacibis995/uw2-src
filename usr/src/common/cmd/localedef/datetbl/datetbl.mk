#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)localedef:common/cmd/localedef/datetbl/datetbl.mk	1.1.8.2"

include $(CMDRULES)

#	Makefile for datetbl

OWN = bin
GRP = bin

all: $(MAINS)

install: all
	$(INS) -f $(USRLIB)/locale/C -m 0555 -u $(OWN) -g $(GRP) time_C
	$(INS) -f $(USRLIB)/locale/C LC_TIME
	$(INS) -f $(USRLIB)/locale/POSIX -m 0555 -u $(OWN) -g $(GRP) time_POSIX
	$(INS) -f $(USRLIB)/locale/POSIX LC_TIME
