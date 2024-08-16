#ident	"@(#)ucb:common/ucbcmd/sendmail/cf/Makefile	1.5"
#ident	"$Header: $"
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#



#		PROPRIETARY NOTICE (Combined)
#
#This source code is unpublished proprietary information
#constituting, or derived under license from AT&T's UNIX(r) System V.
#In addition, portions of such source code were derived from Berkeley
#4.3 BSD under license from the Regents of the University of
#California.
#
#
#
#		Copyright Notice 
#
#Notice of copyright on this source code product does not indicate 
#publication.
#
#	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#	          All rights reserved.

######################################################################
#
#	Makefile for Sendmail configuration files
#
######################################################################

include $(CMDRULES)

SUNSRCS=main.mc \
	subsidiary.mc
GENSRCS=base.m4 \
	compat.m4 \
	ddnm.m4 \
	etherm.m4 \
	localm.m4 \
	uucpm.m4 \
	version.m4 \
SRCS=	$(GENSRCS)

SUNALL=	main.cf \
	subsidiary.cf
ALL=	$(SUNALL)

GET=	sccs get

.DEFAULT:
#	sccs get -G$@ $@

.SUFFIXES: .mc .cf

.mc.cf:
	$(M4) $*.mc > $*.cf

all: $(ALL)

############################
#   special dependencies   #
############################

# all Sun hosts
$(SUNALL): sunbase.m4

# pessimize -- basic dependencies
$(ALL): etherm.m4 base.m4 localm.m4 ddnm.m4 version.m4

# hosts using UUCP
main.cf: uucpm.m4 compat.m4
subsidiary.cf: uucpm.m4 compat.m4

# vanilla dependencies
main.cf:	main.mc
subsidiary.cf:	subsidiary.mc

#####################
#   administrivia   #
#####################

sources: $(SRCS)

clean:
	rm -f $(ALL)

clobber: clean
