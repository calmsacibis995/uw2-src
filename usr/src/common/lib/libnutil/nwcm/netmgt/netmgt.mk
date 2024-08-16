#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libnwutil:common/lib/libnutil/nwcm/netmgt/netmgt.mk	1.5"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"$Id: netmgt.mk,v 1.6.2.1 1994/10/19 18:23:53 vtag Exp $"

include $(LIBRULES)

include ../../local.def

LOCALINC = \
		-DNWCM_FRIEND \
		-I../../nls/English/netmgt \
		$(PICFLAG)

LINTFLAGS = $(WORKINC) $(LOCALDEF) $(GLOBALINC) $(LOCALINC) -c

LINTSUBDIRS =	../tools

DOMAINFILE	= MSG_DOMAIN_NETMGT_FILE
DOMAINREV	= MSG_NETMGT_REV_STR
DOMAIN		= MSG_DOMAIN_NWUMD_DH

BINFILE		= netmgt.bin
BINBUILDER	= mgtbinbuild
SCHEMAFILE	= netmgt_schema.c
SCHEMASOURCE	= netmgt.schema
SCOMP		= ../tools/scomp
BINBUILD	= ../tools/binbuild.o
LINKLIB		= nwnetval
LIBDEST		= ../nwnet

SRCS	    = netmgt_schema.c
OBJS        = $(SRCS:.c=.o)

TARGETS	=	\
	$(SCHEMAFILE) \
	$(OBJS)

SUBDIRS = $(LINTSUBDIRS)

.MUTEX::	$(SCOMP) $(SCHEMAFILE) $(OBJS)

all:	$(TARGETS) $(BINFILE)

install: all
	$(INS) -f $(ETC)/netware/conf -m 0644 $(BINFILE)

$(SCHEMAFILE):  $(SCOMP) $(SCHEMASOURCE)
	$(SCOMP) -o $(SCHEMAFILE) -f $(DOMAINFILE) -r $(DOMAINREV) -d $(DOMAIN) $(SCHEMASOURCE)

$(SCOMP):
	(cd ../tools ; make -f *.mk all)

$(OBJS): $(SRCS)
	$(HCC) -c $(CFLAGS) $(DEFLIST) $(SRCS)

$(BINFILE): $(BINBUILD) $(OBJS)
	$(HCC) -o $(BINBUILDER) $(OBJS) $(BINBUILD)
	./$(BINBUILDER)
	mv schema.bin $(BINFILE)

clean:
	rm -f $(SCHEMAFILE) a.out core errs
	rm -f *.o
	rm -f *.ln

clobber:	clean
	rm -f $(BINFILE)
	rm -f $(BINBUILDER)

lintit:	

