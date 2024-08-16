#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libnwutil:common/lib/libnutil/nls/English/nwnet/nwnet.mk	1.2"
#ident	"$Id: nwnet.mk,v 1.2 1994/10/04 22:28:00 vtag Exp $"

include $(LIBRULES)

GENCAT = gencat

M4BUILD		=	../npub.m4 \
				../pub.m4 \
				../nwumacro.m4

# Files for NWCM Messages
CATNWCM		= nwcmmsgs.cat
MSGNWCM		= nwcmmsgs.msg
M4NWCM		= nwcmmsg.m4
HNWCM		= nwcmmsgs.h
M4LISTNWCM	= $(M4BUILD) \
			  $(M4NWCM) \
			  nwcm.m4 \
			  nwcmfold.m4 \
			  nwnetdh.m4

# Files for npx util messages
CATNPS 		= 	npsmsgs.cat
MSGNPS 		=	npsmsgs.msg
M4NPS		=	npsmsg.m4
HNPS 		= 	npsmsgtable.h
M4LISTNPS	= 	$(M4NPS) \
				$(M4BUILD)

# Files for util library messages 
CATUTIL     =   utilmsgs.cat
MSGUTIL     =   utilmsgs.msg
M4UTIL      =   utilmsg.m4
HUTIL       =   utilmsgtable.h
M4LISTUTIL  =   $(M4NPS) \
                $(M4BUILD) \
                utilmsg.m4 \
                nwcm.m4 \
                nwsapl.m4


TARGETS = $(CATNWCM) $(HNWCM) $(CATNPS) $(HNPS) $(CATUTIL) $(HUTIL)

INS_TARGET1 = $(CATNWCM) $(CATNPS) $(CATNWCM).m $(CATNPS).m \
					$(CATUTIL) $(CATUTIL).m

INS_TARGET2 = $(MSGNWCM) $(MSGNPS) $(MSGUTIL)

INS_TARGET3 = $(HNWCM) $(HNPS) $(HUTIL)

all: $(TARGETS)

install: all
	@-[ -d $(USRLIB)/locale/C/LC_MESSAGES ] || mkdir -p $(USRLIB)/locale/C/LC_MESSAGES
	@for i in $(INS_TARGET1) ; \
	do \
	$(INS) -f $(USRLIB)/locale/C/LC_MESSAGES -m 0444 $$i ; \
	done
	@-[ -d $(USRLIB)/locale/C/MSGFILES ] || mkdir -p $(USRLIB)/locale/C/MSGFILES
	@for i in $(INS_TARGET2) ; \
	do \
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 0444 $$i ; \
	done
	@for i in $(INS_TARGET3) ; \
	do \
	$(INS) -f $(ROOT)/$(MACH)/usr/include -m 755 $$i; \
	done

$(CATNWCM): $(M4LISTNWCM)
	$(RM) -f $(CATNWCM) $(MSGNWCM) 
	$(M4) -DCAT $(M4NWCM) > $(MSGNWCM)  
	$(GENCAT) $(CATNWCM) $(MSGNWCM)

$(HNWCM):: $(M4LISTNWCM)
	$(RM) -f $(HNWCM)
	@echo "	$(M4)  $(M4NWCM) > $(HNWCM)"
	$(M4) $(M4NWCM) > $(HNWCM) 

$(CATNPS): $(M4LISTNPS)
	$(RM) -f $(CATNPS) $(MSGNPS)
	@echo "	$(M4)  -DCAT $(M4NPS) > $(MSGNPS)"
	$(M4) -DCAT $(M4NPS) > $(MSGNPS)
	@echo "	$(GENCAT)  $(CATNPS) $(MSGNPS)"
	$(GENCAT) $(CATNPS) $(MSGNPS)

$(HNPS):: $(M4LISTNPS)
	$(RM) -f $(HNPS)
	@echo "	$(M4)  $(M4NPS) > $(HNPS)"
	$(M4) $(M4NPS) > $(HNPS)

$(CATUTIL): $(M4LISTUTIL)
	$(RM) -f $(CATUTIL) $(MSGUTIL)
	@echo " $(M4)  -DCAT $(M4UTIL) > $(MSGUTIL)"
	$(M4) -DCAT $(M4UTIL) > $(MSGUTIL)
	@echo   $(GENCAT)  $(CATUTIL) $(MSGUTIL)"
	$(GENCAT) $(CATUTIL) $(MSGUTIL)

$(HUTIL):: $(M4LISTUTIL)
	$(RM) -f $(HUTIL)
	@echo " $(M4)  $(M4UTIL) > $(HUTIL)"
	$(M4) $(M4UTIL) > $(HUTIL)

clean:
	$(RM) -f $(CATNWCM) $(HNWCM) $(MSGNWCM)
	$(RM) -f $(CATNPS) $(HNPS) $(MSGNPS)
	$(RM) -f $(CATUTIL) $(HUTIL) $(MSGUTIL)
	$(RM) -f $(CATNWCM).m $(CATNPS).m $(CATUTIL).m

clobber:	clean
	for i in $(INS_TARGET1) ; \
	do \
	$(RM) -fr $(USRLIB)/locale/C/LC_MESSAGES/$$i ; \
	done
	for i in $(INS_TARGET2) ; \
	do \
	$(RM) -fr $(USRLIB)/locale/C/MSGFILES/$$i ; \
	done

lintit:

