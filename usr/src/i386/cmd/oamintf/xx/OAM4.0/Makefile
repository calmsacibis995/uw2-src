#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:i386/cmd/oamintf/xx/OAM4.0/Makefile	1.4"
#ident	"$Header: $"

include $(CMDRULES)

INTF_INSTALL = $(ROOT)/$(MACH)/var/sadm/pkg/oam/save/intf_install
OAMBASE  = $(USRSADM)/sysadm
GRP=bin
OWN=bin

all: .colorpref

.colorpref:
	@echo "====== Making MI and colorpref files"
	grep -v "^#ident	" colorpref > .colorpref

install: all
	[ -d $(OAMBASE)/pref ] || mkdir -p $(OAMBASE)/pref
	[ -d $(INTF_INSTALL) ] || mkdir -p $(INTF_INSTALL)
	$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(OAMBASE)/pref .colorpref
	$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(INTF_INSTALL) oam.mi
	$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(INTF_INSTALL) file.mi
	$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(INTF_INSTALL) devices.mi

clean:
	@echo "nothing to clean"

clobber: clean
	@echo "====== Clobbering MI and colorpref files"
	rm -f .colorpref

size strip lintit:
