#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/etc/etc.mk	1.9.11.3"
#ident	"$Header: $"

#
#	STREAMware TCP
#	Copyright 1987, 1993 Lachman Technology, Inc.
#	All Rights Reserved.
#

include $(CMDRULES)

OWN=		root
GRP=		sys
INITD=		$(ETC)/init.d
INET=		$(ETC)/inet
SINET=		/etc/inet
STARTINET=	$(ETC)/rc2.d/S69inet
STOPINET1=	$(ETC)/rc1.d/K69inet
STOPINET0=	$(ETC)/rc0.d/K69inet
CONFNET=	$(ETC)/confnet.d
INETCONF=	$(CONFNET)/inet
DIRS = 		$(INET) $(ETC)/rc2.d $(ETC)/rc1.d $(ETC)/rc0.d \
			$(CONFNET) $(INETCONF) $(INITD)

# don't include inetinit in FILES, since it gets installed in a different dir
FILES=		hosts inetd.conf networks protocols \
		services shells strcf

all:		$(FILES) init.d/inetinit inet/rc.inet inet/inet.priv \
			inet/listen.setup inet/menu inet/rc.restart \
			inet/named.boot.samp inet/if.ignore \
			inet/ppphosts.samp inet/pppauth.samp inet/config \
			confnet.d/inet/interface confnet.d/inet/config.boot.sh \
			confnet.d/inet/configure


install:	$(DIRS) all
		for i in $(FILES);\
		do\
			$(INS) -f $(INET) -m 0444 -u $(OWN) -g $(GRP) $$i;\
			rm -f $(ETC)/$$i;\
			$(SYMLINK) $(SINET)/$$i $(ETC)/$$i;\
		done
		$(INS) -f $(INET) -m 0540 -u $(OWN) -g $(GRP) inet/inet.priv
		$(INS) -f $(INET) -m 0444 -u $(OWN) -g $(GRP) inet/rc.inet
		$(INS) -f $(INET) -m 0755 -u $(OWN) -g $(GRP) inet/listen.setup
		$(INS) -f $(INET) -m 0755 -u $(OWN) -g $(GRP) inet/menu
		$(INS) -f $(INET) -m 0755 -u $(OWN) -g $(GRP) inet/rc.restart
		$(INS) -f $(INET) -m 0755 -u $(OWN) -g $(GRP) inet/named.boot.samp
		$(INS) -f $(INET) -m 0755 -u $(OWN) -g $(GRP) inet/if.ignore
		$(INS) -f $(INET) -m 0755 -u $(OWN) -g $(GRP) inet/ppphosts.samp
		$(INS) -f $(INET) -m 0755 -u $(OWN) -g $(GRP) inet/pppauth.samp
		$(INS) -f $(INET) -m 0755 -u $(OWN) -g $(GRP) inet/config
		$(INS) -f $(INITD) -m 0444 -u $(OWN) -g $(GRP) init.d/inetinit
		rm -f $(STARTINET) $(STOPINET1) $(STOPINET0)
		-ln $(INITD)/inetinit $(STARTINET)
		-ln $(INITD)/inetinit $(STOPINET1)
		-ln $(INITD)/inetinit $(STOPINET0)
		$(INS) -f $(INETCONF) -m 0644 -u $(OWN) -g $(GRP) \
			confnet.d/inet/interface
		$(INS) -f $(INETCONF) -m 0444 -u $(OWN) -g $(GRP) \
			confnet.d/inet/config.boot.sh
		$(INS) -f $(INETCONF) -m 0544 -u $(OWN) -g $(GRP) \
			confnet.d/inet/configure

$(DIRS):
		[ -d $@ ] || mkdir -p $@

clean:

clobber:

lintit:
