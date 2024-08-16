#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)rcmd.cmd:rcmd.d.mk	1.4"
#ident "$Header: $"

include $(CMDRULES)

LOCALDEF = 

COMMANDS= nrexecd nwnetd

all:	dir

dir:
	@for i in $(COMMANDS);\
	do\
		if [ -d $$i ];then\
			cd  $$i;\
			echo "====== $(MAKE) -f $$i.mk" ; \
			$(MAKE) -f $$i.mk $(MAKEARGS) ; \
			cd .. ; \
		fi;\
	done

install: 
	@for i in $(COMMANDS); \
	do \
		if [ -d $$i ];then\
			cd $$i; \
			$(MAKE) -f $$i.mk install $(MAKEARGS); \
			cd ..; \
		fi; \
	done

	
clean:
	@for i in $(COMMANDS); \
	do \
		if [ -d $$i ];then\
			cd $$i; \
			$(MAKE) -f $$i.mk clean $(MAKEARGS); \
			cd ..; \
		fi; \
	done

clobber:
	@for i in $(COMMANDS); \
	do \
		if [ -d $$i ];then\
			cd $$i; \
			$(MAKE) -f $$i.mk clobber $(MAKEARGS); \
			cd ..; \
		fi; \
	done

lintit:
	@for i in $(COMMANDS); \
	do \
		if [ -d $$i ];then\
			cd $$i; \
			$(MAKE) -f $$i.mk lintit $(MAKEARGS); \
			cd ..; \
		fi; \
	done
