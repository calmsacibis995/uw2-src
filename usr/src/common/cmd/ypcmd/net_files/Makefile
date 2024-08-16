#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ypcmd:net_files/Makefile	1.2.8.7"
#ident  "$Header: $"


#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#	PROPRIETARY NOTICE (Combined)
#
# This source code is unpublished proprietary information
# constituting, or derived under license from AT&T's UNIX(r) System V.
# In addition, portions of such source code were derived from Berkeley
# 4.3 BSD under license from the Regents of the University of
# California.
#
#
#
#	Copyright Notice 
#
# Notice of copyright on this source code product does not indicate 
#  publication.
#
#	(c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#          All rights reserved.
# 

#	Copyright (c) 1992 Intel Corp.
#	  All Rights Reserved
#
#	INTEL CORPORATION PROPRIETARY INFORMATION
#
#	This software is supplied under the terms of a license
#	agreement or nondisclosure agreement with Intel Corpo-
#	ration and may not be copied or disclosed except in
#	accordance with the terms of that agreement.
#

DIR =/etc
DOM = `domainname`
NOPUSH = ""
YPDIR=/usr/sbin
YPDBDIR=/var/yp
YPPUSH=$(YPDIR)/yppush
MAKEDBM=$(YPDIR)/makedbm
MKNETID=$(YPDBDIR)/mknetid
REVNETGROUP=$(YPDBDIR)/revnetgroup
STDETHERS=$(YPDBDIR)/stdethers
STDHOSTS=$(YPDBDIR)/stdhosts
MKALIAS=$(YPDBDIR)/mkalias
YPSHAD2PWD=$(YPDBDIR)/ypshad2pwd
ALIASES = /etc/aliases
YPSERVERS=$(YPDBDIR)/ypservers
ECHO=pfmt -s nostd -g Makefile:

k:
	@if [ ! $(NOPUSH) ]; then $(MAKE)  $(MFLAGS) -k all; \
	else $(MAKE) $(MFLAGS) -k all NOPUSH=$(NOPUSH);fi

#all: ypservers passwd group hosts ethers networks rpc services protocols \
#	netgroup aliases publickey netid netmasks TIMEZONE auto.master \
#	auto.home bootparams
all: ypservers passwd group hosts  networks rpc services protocols \
	publickey netid TIMEZONE  

passwd.time: $(DIR)/passwd $(DIR)/shadow
	@($(YPSHAD2PWD) $(DIR)/passwd $(DIR)/shadow ) | \
	(awk 'BEGIN { FS=":"; OFS="\t"; } /^[a-zA-Z0-9_]/ { print $$1, $$0 }' \
	)| $(MAKEDBM) - $(YPDBDIR)/$(DOM)/passwd.byname; 
	@($(YPSHAD2PWD) $(DIR)/passwd $(DIR)/shadow ) | \
	(awk 'BEGIN { FS=":"; OFS="\t"; } /^[a-zA-Z0-9_]/ { printf("%-10d ", $$3); print $$0 }' \
	)| $(MAKEDBM) - $(YPDBDIR)/$(DOM)/passwd.byuid; 
	@touch passwd.time; 
	@$(ECHO)1 "updated passwd\n"; 
	@if [ ! $(NOPUSH) ]; then \
		$(YPPUSH) -d $(DOM) passwd.byname; \
		$(YPPUSH) -d $(DOM) passwd.byuid; \
		$(ECHO)2 "pushed passwd\n"; \
	fi

group.time: $(DIR)/group
	@(sed -e "/^+/d" -e "/^-/d" $(DIR)/group ) | \
	(awk 'BEGIN { FS=":"; OFS="\t"; } { print $$1, $$0 }' )| $(MAKEDBM) - $(YPDBDIR)/$(DOM)/group.byname; 
	@(sed -e "/^+/d" -e "/^-/d" $(DIR)/group ) | \
	(awk 'BEGIN { FS=":"; OFS="\t"; } { printf("%-10d ", $$3); print $$0 }' ) | $(MAKEDBM) - $(YPDBDIR)/$(DOM)/group.bygid; 
	@touch group.time; 
	@$(ECHO)3 "updated group\n"; 
	@if [ ! $(NOPUSH) ]; then \
		$(YPPUSH) -d $(DOM) group.byname; \
		$(YPPUSH) -d $(DOM) group.bygid; \
		$(ECHO)4 "pushed group\n"; \
	fi

hosts.time: $(DIR)/hosts
	@(sed -e "/^#/d" -e s/#.*$$// $(DIR)/hosts ) | \
	($(STDHOSTS) ) | \
	(awk '{for (i = 2; i <= NF; i++) print $$i, $$0}' )| \
	$(MAKEDBM) $(B)  - $(YPDBDIR)/$(DOM)/hosts.byname 
	@($(STDHOSTS) $(DIR)/hosts )| \
	(awk 'BEGIN { OFS="\t"; } $$1 !~ /^#/ { print $$1, $$0 }' ) | \
	$(MAKEDBM) $(B) - $(YPDBDIR)/$(DOM)/hosts.byaddr; 
	@touch hosts.time; 
	@$(ECHO)5 "updated hosts\n";
	@if [ ! $(NOPUSH) ]; then \
		$(YPPUSH) -d $(DOM) hosts.byname; \
		$(YPPUSH) -d $(DOM) hosts.byaddr; \
		$(ECHO)6 "pushed hosts\n"; \
	fi

ethers.time: $(DIR)/ethers
	@if [ -f $(DIR)/ethers ]; then \
		($(STDETHERS) $(DIR)/ethers ) \
		| (awk '{print $$1, $$0; for (i = 3;i <= NF;i++) print $$i,$$0}' ) \
		| $(MAKEDBM) - $(YPDBDIR)/$(DOM)/ethers.byaddr; \
		(awk 'BEGIN { OFS="\t"; } $$1 !~ /^#/ { print $$2, $$0 }' \
		$(DIR)/ethers ) | \
		$(MAKEDBM) - $(YPDBDIR)/$(DOM)/ethers.byname; \
		touch ethers.time; \
		$(ECHO)7 "updated ethers\n"; \
		if [ ! $(NOPUSH) ]; then \
			$(YPPUSH) -d $(DOM) ethers.byname; \
			$(YPPUSH) -d $(DOM) ethers.byaddr; \
			$(ECHO)8 "pushed ethers\n"; \
		fi \
	else \
		$(ECHO)9 "couldn't find $(DIR)/ethers\n"; \
	fi

networks.time: $(DIR)/networks
	@(sed -e "/^$$/d" -e "/^#/d" -e s/#.*$$// $(DIR)/networks ) | \
	(awk '{print $$1, $$0; for (i = 3;i <= NF;i++) print $$i,$$0}' )| \
	$(MAKEDBM) - $(YPDBDIR)/$(DOM)/networks.byname; 
	@(sed -e "/^$$/d" -e "/^#/d" -e s/#.*$$// $(DIR)/networks ) | \
	(awk 'BEGIN { OFS="\t"; } $$1 !~ /^#/ { print $$2, $$0 }' ) | \
	$(MAKEDBM) - $(YPDBDIR)/$(DOM)/networks.byaddr;
	@touch networks.time; 
	@$(ECHO)10 "updated networks\n"; 
	@if [ ! $(NOPUSH) ]; then \
		$(YPPUSH) -d $(DOM) networks.byname; \
		$(YPPUSH) -d $(DOM) networks.byaddr; \
		$(ECHO)11 "pushed networks\n"; \
	fi

services.time: $(DIR)/services
	@(sed -e "/^$$/d" $(DIR)/services ) | \
	(awk 'BEGIN { OFS="\t"; } $$1 !~ /^#/ { print $$2, $$0 }' ) | \
	$(MAKEDBM) - $(YPDBDIR)/$(DOM)/services.byname;
	@touch services.time; 
	@$(ECHO)12 "updated services\n"; 
	@if [ ! $(NOPUSH) ]; then \
		$(YPPUSH) -d $(DOM) services.byname; \
		$(ECHO)13 "pushed services\n"; \
	fi

rpc.time: $(DIR)/rpc
	@(awk 'BEGIN { OFS="\t"; } $$1 !~ /^#/ { print $$2, $$0 }' \
	    $(DIR)/rpc )| $(MAKEDBM) - $(YPDBDIR)/$(DOM)/rpc.bynumber;
	@touch rpc.time; 
	@$(ECHO)14 "updated rpc\n"; 
	@if [ ! $(NOPUSH) ]; then \
		$(YPPUSH) -d $(DOM) rpc.bynumber; \
		$(ECHO)15 "pushed rpc\n"; \
	fi

protocols.time: $(DIR)/protocols
	@(sed -e "/^$$/d" -e "/^#/d" -e s/#.*$$// $(DIR)/protocols ) | \
	(awk 'BEGIN { OFS="\t"; } $$1 !~ /^#/ { print $$2, $$0 }' ) | \
	$(MAKEDBM) - $(YPDBDIR)/$(DOM)/protocols.bynumber; 
	@(sed -e "/^$$/d" -e "/^#/d" -e s/#.*$$// $(DIR)/protocols ) | \
	(awk '{print $$1,$$0; for (i = 3;i <= NF;i++) print $$i, $$0}' \
	)| $(MAKEDBM) - $(YPDBDIR)/$(DOM)/protocols.byname; 
	@touch protocols.time; 
	@$(ECHO)16 "updated protocols\n"; 
	@if [ ! $(NOPUSH) ]; then \
		$(YPPUSH) -d $(DOM) protocols.byname; \
		$(YPPUSH) -d $(DOM) protocols.bynumber; \
		$(ECHO)17 "pushed protocols\n"; \
	fi

netgroup.time: $(DIR)/netgroup
	@if [ -f $(DIR)/netgroup ]; then \
		$(MAKEDBM) $(DIR)/netgroup $(YPDBDIR)/$(DOM)/netgroup; \
		($(REVNETGROUP) < $(DIR)/netgroup -u ) | \
		$(MAKEDBM) - $(YPDBDIR)/$(DOM)/netgroup.byuser; \
		($(REVNETGROUP) < $(DIR)/netgroup -h ) | \
		$(MAKEDBM) - $(YPDBDIR)/$(DOM)/netgroup.byhost; \
		touch netgroup.time; \
		$(ECHO)18 "updated netgroup\n"; \
		if [ ! $(NOPUSH) ]; then \
			$(YPPUSH) -d $(DOM) netgroup; \
			$(YPPUSH) -d $(DOM) netgroup.byuser; \
			$(YPPUSH) -d $(DOM) netgroup.byhost; \
			$(ECHO)19 "pushed netgroup\n"; \
		fi \
	else \
		$(ECHO)20 "couldn't find $(DIR)/netgroup\n"; \
	fi

aliases.time: $(ALIASES)
	@if [ -f $(ALIASES) ]; then \
		cp $(ALIASES) $(YPDBDIR)/$(DOM)/mail.aliases; \
		/usr/ucblib/sendmail -bi -oA$(YPDBDIR)/$(DOM)/mail.aliases; \
		$(MKALIAS) $(YPDBDIR)/$(DOM)/mail.aliases $(YPDBDIR)/$(DOM)/mail.byaddr; \
		(sed -e "/^#/d" -e s/#.*$$// $(ALIASES)) | \
		awk -F: '{print$$1"\t"$$2}' | $(MAKEDBM) - $(YPDBDIR)/$(DOM)/mail.byname; \
		rm $(YPDBDIR)/$(DOM)/mail.aliases; \
		touch aliases.time; \
		$(ECHO)21 "updated aliases\n"; \
		if [ ! $(NOPUSH) ]; then \
			$(YPPUSH) -d $(DOM) mail.aliases; \
			$(YPPUSH) -d $(DOM) mail.byaddr; \
			$(ECHO)22 "pushed aliases\n"; \
		fi \
	else \
		$(ECHO)23 "couldn't find $(ALIASES)\n"; \
	fi

netmasks.time: $(DIR)/netmasks
	@if [ -f $(DIR)/netmasks ]; then \
		(sed -e "/^$$/d" -e "/^#/d" -e s/#.*$$// $(DIR)/netmasks ) | \
		$(MAKEDBM) - $(YPDBDIR)/$(DOM)/netmasks.byaddr; \
		touch netmasks.time; \
		$(ECHO)24 "updated netmasks\n"; \
		if [ ! $(NOPUSH) ]; then \
			$(YPPUSH) -d $(DOM) netmasks.byaddr; \
			$(ECHO)25 "pushed netmasks\n"; \
		fi \
	else \
		$(ECHO)26 "couldn't find $(DIR)/netmasks\n"; \
	fi

publickey.time: $(DIR)/publickey
	@(sed "/^#/d" < $(DIR)/publickey )| $(MAKEDBM) - $(YPDBDIR)/$(DOM)/publickey.byname; 
	@touch publickey.time; 
	@$(ECHO)27 "updated publickey\n"; 
	@if [ ! $(NOPUSH) ]; then \
		$(YPPUSH) -d $(DOM) publickey.byname; \
		$(ECHO)28 "pushed publickey\n"; \
	fi

netid.time: $(DIR)/passwd $(DIR)/group $(DIR)/hosts $(DIR)/netid
	@$(MKNETID) -q -d $(DOM) -p $(DIR)/passwd -g $(DIR)/group -h $(DIR)/hosts -m $(DIR)/netid > .ypjunk; 
	@$(MAKEDBM) .ypjunk $(YPDBDIR)/$(DOM)/netid.byname; 
	@rm -f .ypjunk; 
	@touch netid.time; 
	@$(ECHO)29 "updated netid\n"; 
	@if [ ! $(NOPUSH) ]; then \
		$(YPPUSH) -d $(DOM) netid.byname; \
		$(ECHO)30 "pushed netid\n"; \
	fi

bootparams.time: $(DIR)/bootparams
	@if [ -f $(DIR)/bootparams ]; then \
		(sed -e '/^#/d' -e s/#.*$$// -e 's/[    ][  ]*$$//' \
			-e '/\\$$/s/\\$$/ /' $(DIR)/bootparams )\
		|( awk '/ $$/ {printf "%s", $$0} !/ $$/ {print}' )\
		|( sed -e 's/[   ][  ]*/ /g' )\
		| $(MAKEDBM) - $(YPDBDIR)/$(DOM)/bootparams; \
		touch bootparams.time; \
		$(ECHO)31 "updated bootparams\n"; \
		if [ ! $(NOPUSH) ]; then \
			$(YPPUSH) -d $(DOM) bootparams; \
			$(ECHO)32 "pushed bootparams\n"; \
		fi \
	else \
		$(ECHO)33 "couldn't find $(DIR)/bootparms\n"; \
	fi

TIMEZONE.time:  $(DIR)/TIMEZONE
	@if [ -f $(DIR)/TIMEZONE ]; then \
		sed -e "/^#/d" -e s/#.*$$// $(DIR)/TIMEZONE \
		| awk '{for (i = 2; i<=NF; i++) print $$i, $$0}' \
		| $(MAKEDBM) - $(YPDBDIR)/$(DOM)/TIMEZONE.byname; \
		touch TIMEZONE.time; \
		$(ECHO)34 "updated TIMEZONE\n"; \
		if [ ! $(NOPUSH) ]; then \
			$(YPPUSH) TIMEZONE.byname; \
			$(ECHO)35 "pushed TIMEZONE\n"; \
		fi \
	else \
	   $(ECHO)36 "couldn't find $(DIR)/TIMEZONE\n"; \
	fi
 
auto.master.time:  $(DIR)/auto.master
	@if [ -f $(DIR)/auto.master ]; then \
		sed -e "/^#/d" -e s/#.*$$// $(DIR)/auto.master \
		| $(MAKEDBM) - $(YPDBDIR)/$(DOM)/auto.master; \
		touch auto.master.time; \
		$(ECHO)37 "updated auto.master\n"; \
		if [ ! $(NOPUSH) ]; then \
			$(YPPUSH) auto.master; \
			$(ECHO)38 "pushed auto.master\n"; \
		fi \
	else \
		$(ECHO)39 "couldn't find $(DIR)/auto.master\n"; \
	fi  
 
auto.home.time:  $(DIR)/auto.home
	@if [ -f $(DIR)/auto.home ]; then \
		sed -e "/^#/d" -e s/#.*$$// $(DIR)/auto.home \
		| $(MAKEDBM) - $(YPDBDIR)/$(DOM)/auto.home; \
		touch auto.home.time; \
		$(ECHO)40 "updated auto.home\n"; \
		if [ ! $(NOPUSH) ]; then \
			$(YPPUSH) auto.home; \
			$(ECHO)41 "pushed auto.home\n"; \
		fi \
	else \
		$(ECHO)42 "couldn't find $(DIR)/auto.home\n"; \
	fi

ypservers.time:
	@cp $(YPDBDIR)/binding/$(DOM)/ypservers $(YPSERVERS)
	@(sed -e "/^$$/d" -e "/^#/d" -e s/#.*$$// $(YPSERVERS) ) | \
	(awk '{print $$0, $$0}' ) | grep -v "\*" | grep -v "+" |\
	$(MAKEDBM) - $(YPDBDIR)/$(DOM)/ypservers;
	@rm $(YPSERVERS);
	@touch ypservers.time;
	@$(ECHO)43 "updated ypservers\n";
	@if [ ! $(NOPUSH) ]; then \
		$(YPPUSH) -d $(DOM) ypservers; \
		$(ECHO)44 "pushed ypservers\n"; \
	fi

passwd: passwd.time
group: group.time
hosts: hosts.time
ethers: ethers.time
networks: networks.time
rpc: rpc.time
services: services.time
protocols: protocols.time
netgroup: netgroup.time
aliases: aliases.time
publickey: publickey.time
netid: netid.time
netmasks: netmasks.time
bootparams: bootparams.time
TIMEZONE: TIMEZONE.time
auto.master: auto.master.time
auto.home: auto.home.time
ypservers: ypservers.time
$(DIR)/netid:
$(DIR)/bootparams:
$(DIR)/TIMEZONE:
$(DIR)/auto.master:
$(DIR)/auto.home:
$(DIR)/ethers:
$(DIR)/netgroup:
$(ALIASES):
$(DIR)/netmasks:
