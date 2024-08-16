#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nametoaddr:common/lib/nametoaddr/nametoaddr.mk	1.6.5.10"
#ident	"$Header: $"
#
# makefile for name to address mapping dynamic linking libraries.
#

include $(LIBRULES)

TESTDIR = .
FRC =

all:
	@for i in oam resolv straddr tcpip novell ns tcpip_nis ; do  \
		if [ -d $$i ] ; then \
			cd $$i;\
			echo "##### $(MAKE) -f $$i.mk";\
			$(MAKE) -f $$i.mk $(MAKEARGS);\
			cd ..; \
		fi; \
	done;

install: 
	@for i in oam resolv straddr tcpip novell ns tcpip_nis; do  \
		if [ -d $$i ] ; then \
			cd $$i;\
			echo $$i;\
			echo "##### $(MAKE) install -f $$i.mk";\
			$(MAKE) install -f $$i.mk $(MAKEARGS);\
			cd ..;\
		fi; \
	done;

clean:
	@for i in oam resolv straddr tcpip novell ns tcpip_nis ; do  \
		if [ -d $$i ] ; then \
			cd $$i;\
			echo $$i;\
			$(MAKE) -f $$i.mk clean $(MAKEARGS);\
			cd .. ;\
		fi; \
	done

clobber:
	@for i in oam resolv straddr tcpip novell ns tcpip_nis ; do  \
		if [ -d $$i ] ; then \
			cd $$i;\
			echo $$i;\
			$(MAKE) -f $$i.mk clobber $(MAKEARGS);\
			cd .. ;\
		fi; \
	done

lintit:

FRC:
