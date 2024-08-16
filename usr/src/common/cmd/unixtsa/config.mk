#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)unixtsa:common/cmd/unixtsa/config.mk	1.4"
###
#
#  name		config.mk - configuration makefile for unixtsa
#		@(#)unixtsa:common/cmd/unixtsa/config.mk	1.4	7/18/94
#
###

include $(CMDRULES)

OPTBIN		=	$(ROOT)/$(MACH)/opt/bin
#INC=.
LOCALINC=-I$(TOP)/include
#GLOBALDEF=-DUNIX -DSYSV -DSVR4 -DTLI -DDEBUG
GLOBALDEF=-DUNIX -DSYSV -DSVR4 -DTLI
LIBDIR=$(TOP)/lib
BINDIR=$(USRSBIN)

LIBLIST=-L$(LIBDIR) -lsms -lsocket -lnsl $(LDLIBS) $(PERFLIBS)

lintit:

do_subdirs ::
	@for Dir in $(SUBDIRS); do \
	(cd $$Dir; echo ">>> [`pwd`] $(MAKE) $(MAKEARGS) -f `basename $$Dir.mk` $(MTARG)"; $(MAKE) $(MAKEARGS) -f `basename $$Dir.mk` $(MTARG)) \
	done

# a frequently usable clean rule
do_clean ::
	-$(RM) -f *.o core nohup.out lint.out *.ln *.lint

# a frequently usable clobber rule
do_clobber :: 
	-$(RM) -f $(TARGETS) $(TARGET)

.C.o:
	$(C++C) $(CFLAGS) $(INCLIST) $(DEFLIST) -c $<
