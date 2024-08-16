#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/cmd-inet.mk	1.10.12.6"
#ident	"$Header: $"
#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#

# 
# 
# 		Copyright Notice 
# 
# Notice of copyright on this source code product does not indicate 
# publication.
# 
# 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
#

include $(CMDRULES)

LOCALDEF=	-DSYSV -DSTRNET -DBSD_COMP

YFLAGS = -d

DIRS=		etc usr.lib/libtelnet usr.bin usr.sbin msgs

all install:
	@for i in $(DIRS);\
	do\
		( cd $$i;\
		/bin/echo "\n===== $(MAKE) $(MAKEFLAGS) -f `basename $$i.mk` $@";\
		$(MAKE) -$(MAKEFLAGS) -f `basename $$i.mk` $@ $(MAKEARGS);\
		)\
	done;\
	wait
	[ -d $(ROOT)/$(MACH)/tmp/inet ] || mkdir -p $(ROOT)/$(MACH)/tmp/inet
	$(INS) -f $(ROOT)/$(MACH)/tmp/inet upgrade/rc.merge
	$(INS) -f $(ROOT)/$(MACH)/tmp/inet upgrade/inter.merge
	[ -d $(ROOT)/$(MACH)/etc/inst/save.orig/etc/inet ] || mkdir -p $(ROOT)/$(MACH)/etc/inst/save.orig/etc/inet
	$(INS) -f $(ROOT)/$(MACH)/etc/inst/save.orig/etc/inet upgrade/bootptab
	$(INS) -f $(ROOT)/$(MACH)/etc/inst/save.orig/etc/inet upgrade/hosts
	$(INS) -f $(ROOT)/$(MACH)/etc/inst/save.orig/etc/inet upgrade/if.ignore
	$(INS) -f $(ROOT)/$(MACH)/etc/inst/save.orig/etc/inet upgrade/inetd.conf
	$(INS) -f $(ROOT)/$(MACH)/etc/inst/save.orig/etc/inet upgrade/networks
	$(INS) -f $(ROOT)/$(MACH)/etc/inst/save.orig/etc/inet upgrade/protocols
	$(INS) -f $(ROOT)/$(MACH)/etc/inst/save.orig/etc/inet upgrade/shells

clean lintit:
	@for i in $(DIRS);\
	do\
		( cd $$i;\
		/bin/echo "\n===== $(MAKE) -f `basename $$i.mk` $@";\
		$(MAKE) -$(MAKEFLAGS) -f `basename $$i.mk` $@;\
		)\
	done;\
	wait

clobber: clean
	@for i in $(DIRS);\
	do\
		( cd $$i;\
		/bin/echo "\n===== $(MAKE) $(MAKEFLAGS) -f `basename $$i.mk` $@";\
		$(MAKE) -$(MAKEFLAGS) -f `basename $$i.mk` $@;\
		)\
	done;\
	wait
