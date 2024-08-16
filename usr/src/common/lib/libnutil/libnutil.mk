#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libnwutil:common/lib/libnutil/libnutil.mk	1.16"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"$Id: libnutil.mk,v 1.28 1994/09/28 21:35:08 vtag Exp $"

include $(LIBRULES)

include local.def

MCC = $(CC)

XMODE = 0555

NLS = \
	$(NLSDIR)/nwnet/npsmsgtable.h \
	$(NLSDIR)/nwnet/nwcmmsgs.h \
	$(NLSDIR)/nwnet/utilmsgtable.h \
	$(NLSDIR)/netmgt/netmgtmsgtable.h \
	$(NLSDIR)/nprinter/printmsgtable.h \
	$(NLSDIR)/nuc/nucmsgtable.h

NLSDIR = ./nls/English

NWCMDIR = ./nwcm

LOCALINC = \
		-DNWCM_FRIEND \
		$(PICFLAG) \
		-I./nls/English \
		-I./nls/English/nwnet \
		-I./nls/English/netmgt \
		-I./nls/English/nuc \
		-I./nls/English/nprinter

LINTFLAGS = $(WORKINC) $(LOCALDEF) $(GLOBALINC) $(LOCALINC) -c

LEXFLAGS = -l$(SGSROOT)/usr/ccs/lib/lex/ncform

LIBRARY = nwutil

LIBDEST = .
HFLAGS = -h /usr/lib/lib$(LIBRARY).so
MYLDFLAGS = -G -dy -ztext

SRCS = \
		initcfg.c \
		logpid.c \
		nwalloc.c \
		microsleep.c \
		msg.c \
		nwutil_mt.c \
		nps.c \
		nwcm.c \
		nwconfig.c \
		sap_dos.c \
		sap_err.c \
		sap_lib.c \
		server.c \
		sapreq.c
LIBYSRCS = cmgram.y	
LIBLSRCS = cmscan.l	

OBJS = \
	$(SRCS:.c=.o) \
	$(LIBYSRCS:.y=.o) \
	$(LIBLSRCS:.l=.o)

.MUTEX::	$(NLS) $(OBJS) libs

all: $(NLS) libs
	cd $(NWCMDIR); \
	$(MAKE) -f *.mk $@ $(MAKEARGS)

install: all
	cd $(NLSDIR); \
	$(MAKE) -f *.mk $@ $(MAKEARGS)
	cd $(NWCMDIR); \
	$(MAKE) -f *.mk $@ $(MAKEARGS)
	$(INS) -f $(USRLIB) -m 0644 $(LIBDEST)/lib$(LIBRARY).a
	$(INS) -f $(USRLIB) -m 0644 $(LIBDEST)/lib$(LIBRARY).so

libs:  $(LIBDEST)/lib$(LIBRARY).a $(LIBDEST)/lib$(LIBRARY).so

clean clobber::
	cd $(NLSDIR); \
	$(MAKE) -f *.mk $@ $(MAKEARGS)
	cd $(NWCMDIR); \
	$(MAKE) -f *.mk $@ $(MAKEARGS)

$(NLS):
	cd $(NLSDIR); \
	$(MAKE) -f *.mk install $(MAKEARGS)

$(LIBDEST)/lib$(LIBRARY).a:	$(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

$(LIBDEST)/lib$(LIBRARY).so: $(OBJS)
	rm -f $(LIBDEST)/lib$(LIBRARY).r $(LIBDEST)/lib$(LIBRARY).so
	$(LD) -r -o $(LIBDEST)/lib$(LIBRARY).r $(OBJS)
	$(LD) $(MYLDFLAGS) $(LFLAGS) $(HFLAGS) -o $(LIBDEST)/lib$(LIBRARY).so \
		$(OBJS)

clean::
	rm -f *.o
	rm -f *.ln

clobber::
	rm -f $(LIBDEST)/lib$(LIBRARY).a $(LIBDEST)/lib$(LIBRARY).so lint.out core.*
	rm -f *.o
	rm -f *.ln
	rm -f *.r

lintit:: $(SRCS)
	-@rm -f lint.out ; \
	for file in $(SRCS) ;\
	do \
	echo '## lint output for '$$file' ##' >>lint.out ;\
	$(LINT) $(LINTFLAGS) $$file $(LINTLIBS) >>lint.out ;\
	done

lintit:: $(NWCMDIR)
	cd $(NWCMDIR); \
	$(MAKE) -f *.mk $@

$(NLSDIR)/nwnet/npsmsgtable.h: $(NLSDIR)/nwnet/npsmsg.m4 $(NLSDIR)/npub.m4 $(NLSDIR)/nwnet/nwsapl.m4 $(NLSDIR)/nwumacro.m4
$(NLSDIR)/nwnet/nwcmmsgs.h: $(NLSDIR)/npub.m4 $(NLSDIR)/nwnet/nwcm.m4 $(NLSDIR)/nwnet/nwnetdh.m4 $(NLSDIR)/nwnet/nwcmfold.m4 $(NLSDIR)/nwnet/nwcmmsg.m4 $(NLSDIR)/nwumacro.m4
$(NLSDIR)/nwnet/utilmsgtable.h: $(NLSDIR)/npub.m4 $(NLSDIR)/nwnet/nwcm.m4 $(NLSDIR)/nwnet/nwnetdh.m4 $(NLSDIR)/nwnet/nwcmfold.m4 $(NLSDIR)/nwnet/nwcmmsg.m4 $(NLSDIR)/nwumacro.m4
$(NLSDIR)/netmgt/netmgtmsgtable.h: $(NLSDIR)/npub.m4 $(NLSDIR)/nwumacro.m4 $(NLSDIR)/netmgt/netmgtmsg.m4
$(NLSDIR)/nprinter/printmsgtable.h: $(NLSDIR)/npub.m4 $(NLSDIR)/nwumacro.m4 $(NLSDIR)/nprinter/printmsg.m4
$(NLSDIR)/nuc/nucmsgtable.h: $(NLSDIR)/npub.m4 $(NLSDIR)/nwumacro.m4 $(NLSDIR)/nuc/nucdh.m4

