#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libnwutil:common/lib/libnutil/nls/English/nprinter/nprinter.mk	1.2"
#ident	"$Id: nprinter.mk,v 1.2 1994/10/10 17:09:28 mark Exp $"

include $(LIBRULES)

GENCAT = gencat

M4BUILD		=	../npub.m4 \
				../pub.m4 \
				../nwumacro.m4

# Files for Netware printing messages
CATPRT		=	prntmsgs.cat
MSGPRT		=	printmsgs.msg
M4PRT		=	printmsg.m4
HPRT		=	printmsgtable.h
M4LISTPRT	=	$(M4PRT) \
				$(M4BUILD) \
				nprintdh.m4

TARGETS = $(CATPRT) $(HPRT)

INS_TARGET1 = $(CATPRT) $(CATPRT).m

INS_TARGET2 = $(MSGPRT)

INS_TARGET3 = $(HPRT)

all: $(TARGETS)

install: all
	@-[ -d $(USRLIB)/locale/C/LC_MESSAGES ] || mkdir -p $(USRLIB)/locale/C/LC_MESSAGES
	@for i in $(INS_TARGET1) ; \
	do \
	$(INS) -f $(USRLIB)/locale/C/LC_MESSAGES -m 0444 $$i ; \
	done
	@-[ -d $(USRLIB)/locale/C/MSGFILES ] || mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 0444 $(MSGPRT) ; \
	$(INS) -f $(ROOT)/$(MACH)/usr/include -m 755 $(HPRT);

clean:
	$(RM) -f $(CATPRT) $(HPRT) $(MSGPRT)
	$(RM) -f $(CATPRT).m

clobber:	clean
	for i in $(INS_TARGET1) ; \
	do \
	$(RM) -fr $(USRLIB)/locale/C/LC_MESSAGES/$$i ; \
	done
	$(RM) -fr $(USRLIB)/locale/C/MSGFILES/$(MSGPRT);


$(CATPRT): $(M4LISTPRT)
	$(RM) -f $(CATPRT) $(MSGPRT)
	@echo " $(M4)  -DCAT $(M4PRT) > $(MSGPRT)"
	$(M4) -DCAT $(M4PRT) > $(MSGPRT)
	@echo   $(GENCAT)  $(CATPRT) $(MSGPRT)"
	$(GENCAT) $(CATPRT) $(MSGPRT)

$(HPRT):: $(M4LISTPRT)
	$(RM) -f $(HPRT)
	@echo " $(M4)  $(M4PRT) > $(HPRT)"
	$(M4) $(M4PRT) > $(HPRT)

lintit:

