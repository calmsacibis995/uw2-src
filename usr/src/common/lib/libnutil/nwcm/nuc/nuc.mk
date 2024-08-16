#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libnwutil:common/lib/libnutil/nwcm/nuc/nuc.mk	1.6"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"$Id: nuc.mk,v 1.7.2.1 1994/10/19 18:24:03 vtag Exp $"

include $(LIBRULES)

include ../../local.def

LOCALINC = \
		-DNWCM_FRIEND \
		-I../../nls/English/nuc \
		$(PICFLAG)

LINTFLAGS = $(WORKINC) $(LOCALDEF) $(GLOBALINC) $(LOCALINC) -c

LINTSUBDIRS =	\
				../tools

DOMAINFILE		= MSG_DOMAIN_NUC_FILE
DOMAINREV		= MSG_NUC_REV_STR
DOMAIN			= MSG_DOMAIN_NUC_DH

BINFILE			= nuc.bin
BINBUILDER		= nucbinbuild
SCHEMAFILE		= nuc_schema.c
SCHEMASOURCE	= nuc.schema
SCOMP			= ../tools/scomp
BINBUILD		= ../tools/binbuild.o

NWNETSRCS   = nuc_val.c
NWNETOBJS   = \
	$(NWNETSRCS:.c=.o)

SCHEMAOBJ   = \
	$(SCHEMAFILE:.c=.o)

TARGETS	= $(SCHEMAFILE)

LIBRARY = nucval
LIBDEST = .

HFLAGS = -h /usr/lib/lib$(LIBRARY).so
MYLDFLAGS = -G -dy -ztext

SUBDIRS = $(LINTSUBDIRS)

.MUTEX:	$(SCOMP) $(SCHEMAFILE) $(SCHEMAOBJ)

all:	$(TARGETS) libs $(BINFILE)

install: all
	$(INS) -f $(USRLIB) -m 0644 $(LIBDEST)/lib$(LIBRARY).so
	$(INS) -f $(ETC)/netware/conf -m 0644 $(BINFILE)

libs: $(LIBDEST)/lib$(LIBRARY).so

$(SCHEMAFILE): $(SCOMP)
	$(SCOMP) -o $(SCHEMAFILE) -f $(DOMAINFILE) -r $(DOMAINREV) -d $(DOMAIN) -l lib$(LIBRARY).so $(SCHEMASOURCE)

$(SCOMP):
	(cd ../tools ; make -f *.mk all)

$(LIBDEST)/lib$(LIBRARY).so: $(NWNETOBJS)
	rm -f $(LIBDEST)/lib$(LIBRARY).r $(LIBDEST)/lib$(LIBRARY).so
	$(LD) -r -o $(LIBDEST)/lib$(LIBRARY).r $(NWNETOBJS)
	$(LD) $(MYLDFLAGS) $(LFLAGS) $(HFLAGS) -o $(LIBDEST)/lib$(LIBRARY).so \
		$(NWNETOBJS) $(OBJS) -L../.. -lnwutil

$(SCHEMAOBJ): $(SCHEMAFILE)
	$(HCC) -c $(CFLAGS) $(DEFLIST) $(SCHEMAFILE)

$(BINFILE): $(BINBUILD) $(SCHEMAOBJ)
	$(HCC) -o $(BINBUILDER) $(SCHEMAOBJ) $(BINBUILD)
	./$(BINBUILDER)
	mv schema.bin $(BINFILE)

clean:
	rm -f $(SCHEMAFILE) a.out core errs
	rm -f *.o
	rm -f *.ln

clobber:	clean
	rm -f *.so
	rm -f *.h
	rm -f $(BINFILE)
	rm -f $(BINBUILDER)
	rm -f *.r

lintit:	

