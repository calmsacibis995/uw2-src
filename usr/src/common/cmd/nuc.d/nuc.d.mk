#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nuc.cmd:nuc.d.mk	1.3"
#ident "$Header: /SRCS/esmp/usr/src/nw/cmd/nuc.d/nuc.d.mk,v 1.2 1994/07/11 01:37:08 huston Exp $"

include $(CMDRULES)

LOCALDEF = -DIAPX386 -Di386 -DSVR4 -DSVr4 -DSVr4_1

all:	dir

dir:
	@for i in `ls`;\
	do\
		if [ -d $$i ];then\
			cd  $$i;\
			echo "====== $(MAKE) -f $$i.mk" ; \
			$(MAKE) -f $$i.mk $(MAKEARGS) ; \
			cd .. ; \
		fi;\
	done

install: install_cmds install_msgs

install_cmds: 
	@for i in `ls`; \
	do \
		if [ -d $$i ];then\
			cd $$i; \
			$(MAKE) -f $$i.mk install $(MAKEARGS); \
			cd ..; \
		fi; \
	done

install_msgs: uvlnuc.str
	-[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 uvlnuc.str
	

clean:
	@for i in `ls`; \
	do \
		if [ -d $$i ];then\
			cd $$i; \
			$(MAKE) -f $$i.mk clean $(MAKEARGS); \
			cd ..; \
		fi; \
	done

clobber:
	@for i in `ls`; \
	do \
		if [ -d $$i ];then\
			cd $$i; \
			$(MAKE) -f $$i.mk clobber $(MAKEARGS); \
			cd ..; \
		fi; \
	done

lintit:
	@for i in `ls`; \
	do \
		if [ -d $$i ];then\
			cd $$i; \
			$(MAKE) -f $$i.mk lintit $(MAKEARGS); \
			cd ..; \
		fi; \
	done
