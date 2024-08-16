#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libnwutil:common/lib/libnutil/nls/English/netmgt/netmgt.mk	1.2"
#ident	"$Id: netmgt.mk,v 1.2 1994/10/10 17:08:50 mark Exp $"

include $(LIBRULES)

GENCAT = gencat

M4BUILD		=	../npub.m4 \
				../pub.m4 \
				../nwumacro.m4

# Files for Network Management messages
CATNM 		= 	nmmsgs.cat
MSGNM 		=	netmgtmsgs.msg
M4NM		=	netmgtmsg.m4
HNM 		= 	netmgtmsgtable.h
M4LISTNM	= 	$(M4NM) \
				$(M4BUILD) \
				netmgtdh.m4

TARGETS = $(CATNM) $(HNM)

INS_TARGET1 = $(CATNM) $(CATNM).m

INS_TARGET2 = $(MSGNM)

INS_TARGET3 = $(HNM)

all: $(TARGETS)

install: all
	@-[ -d $(USRLIB)/locale/C/LC_MESSAGES ] || mkdir -p $(USRLIB)/locale/C/LC_MESSAGES
	@for i in $(INS_TARGET1) ; \
	do \
	$(INS) -f $(USRLIB)/locale/C/LC_MESSAGES -m 0444 $$i ; \
	done
	@-[ -d $(USRLIB)/locale/C/MSGFILES ] || mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 0444 $(MSGNM) ; \
	$(INS) -f $(ROOT)/$(MACH)/usr/include -m 755 $(HNM);

clean:
	$(RM) -f $(CATNM) $(HNM) $(MSGNM)
	$(RM) -f $(CATNM).m 

clobber:	clean
	for i in $(INS_TARGET1) ; \
	do \
	$(RM) -fr $(USRLIB)/locale/C/LC_MESSAGES/$$i ; \
	done
	$(RM) -fr $(USRLIB)/locale/C/MSGFILES/$(MSGNM);

$(CATNM): $(M4LISTNM)
	$(RM) -f $(CATNM) $(MSGNM)
	@echo "	$(M4)  -DCAT $(M4NM) > $(MSGNM)"
	$(M4) -DCAT $(M4NM) > $(MSGNM)
	@echo "	$(GENCAT)  $(CATNM) $(MSGNM)"
	$(GENCAT) $(CATNM) $(MSGNM)

$(HNM):: $(M4LISTNM)
	$(RM) -f $(HNM)
	@echo "	$(M4)  $(M4NM) > $(HNM)"
	$(M4) $(M4NM) > $(HNM)

lintit:

