#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libnwutil:common/lib/libnutil/nls/English/nuc/nuc.mk	1.1"
#ident	"@(#)libnwutil:nls/English/nuc/nuc.mk	1.2"
#ident	"$Id: nuc.mk,v 1.1 1994/08/30 15:28:47 mark Exp $"

include $(LIBRULES)

GENCAT = gencat

M4BUILD		=	../npub.m4 \
				../pub.m4 \
				../nwumacro.m4

# Files for Netware printing messages
CATNUC		=	nucmsgs.cat
MSGNUC		=	nucmsgs.msg
M4NUC		=	nucdh.m4
HNUC		=	nucmsgtable.h
M4LISTNUC	=	$(M4NUC) \
				$(M4BUILD)

TARGETS = $(CATNUC) $(HNUC)

INS_TARGET1 = $(CATNUC) $(CATNUC).m

INS_TARGET2 = $(MSGNUC)

INS_TARGET3 = $(HNUC)

all: $(TARGETS)

install: all
	@-[ -d $(USRLIB)/locale/C/LC_MESSAGES ] || mkdir -p $(USRLIB)/locale/C/LC_MESSAGES
	@for i in $(INS_TARGET1) ; \
	do \
	$(INS) -f $(USRLIB)/locale/C/LC_MESSAGES -m 0444 $$i ; \
	done
	@-[ -d $(USRLIB)/locale/C/MSGFILES ] || mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 0444 $(MSGNUC) ; \
	$(INS) -f $(ROOT)/$(MACH)/usr/include -m 755 $(HNUC);

clean:
	$(RM) -f $(CATNUC) $(HNUC) $(MSGNUC)
	$(RM) -f $(CATNUC).m

clobber:	clean
	for i in $(INS_TARGET1) ; \
	do \
	$(RM) -fr $(USRLIB)/locale/C/LC_MESSAGES/$$i ; \
	done
	$(RM) -fr $(USRLIB)/locale/C/MSGFILES/$(MSGNUC);

$(CATNUC): $(M4LISTNUC)
	$(RM) -f $(CATNUC) $(MSGNUC)
	@echo " $(M4)  -DCAT $(M4NUC) > $(MSGNUC)"
	$(M4) -DCAT $(M4NUC) > $(MSGNUC)
	@echo   $(GENCAT)  $(CATNUC) $(MSGNUC)"
	$(GENCAT) $(CATNUC) $(MSGNUC)

$(HNUC):: $(M4LISTNUC)
	$(RM) -f $(HNUC)
	@echo " $(M4)  $(M4NUC) > $(HNUC)"
	$(M4) $(M4NUC) > $(HNUC)

lintit:

