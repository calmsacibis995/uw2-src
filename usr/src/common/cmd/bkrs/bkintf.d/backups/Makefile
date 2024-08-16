#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/bkintf.d/backups/Makefile	1.3.9.5"
#ident "$Header: Makefile 1.4 91/09/11 $"

include $(CMDRULES)

SADMDIR=$(USRSADM)
BKBIN=$(SADMDIR)/bkup/bin
OAMBASE=$(SADMDIR)/sysadm
ETCBKUP=$(ETC)/bkup
BACKUPDIR = $(OAMBASE)/add-ons/bkrs/backup_service/extended
SADMVAR=$(VAR)/sadm
PKGDIR=$(SADMVAR)/pkg/bkrs
MIDIR = $(PKGDIR)/save/intf_install
MIFILE = bkup.mi
HELPFILE = Help
PRODUCTS=backup bkhist bkoper bkrem bksched bksetup bkstat
OWN=bin
GRP=bin

backups: all

all: $(PRODUCTS) $(MIFILE) $(HELPFILE)

.sh:
	cp $< $*

$(PRODUCTS):
	cd $(@); $(MAKE) $(MAKEARGS) $(@); cd ..

$(MIFILE):
	grep -v "^#ident" 08455426588.mi >$(MIFILE)

clean:

clobber: clean
	for f in $(PRODUCTS) ;\
	do \
		cd $$f ;\
		$(MAKE) $(MAKEARGS) $(@) ;\
		cd .. ;\
	done
	rm -f $(MIFILE)

size strip lintit:
	for f in $(PRODUCTS) ;\
	do \
		cd $$f ;\
		$(MAKE) $(MAKEARGS) $(@) ;\
		cd .. ;\
	done

install: all $(BACKUPDIR) $(PKGDIR) $(MIDIR)
	for f in $(PRODUCTS) ;\
	do \
		cd $$f ;\
		$(MAKE) $(MAKEARGS) $(@) ;\
		cd .. ;\
	done

	$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(MIDIR) $(MIFILE)
	$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(BACKUPDIR) $(HELPFILE)

$(BACKUPDIR):
	if [ ! -d $(BACKUPDIR) ] ;\
	then \
		mkdir -p $(BACKUPDIR) ;\
	fi
$(PKGDIR):
	if [ ! -d `dirname $(PKGDIR)` ] ;\
	then \
		mkdir -p `dirname $(PKGDIR)` ;\
	fi
	if [ ! -d $(PKGDIR) ] ;\
	then \
		mkdir -p $(PKGDIR) ;\
	fi
$(MIDIR):
	if [ ! -d $(MIDIR) ] ;\
	then \
		mkdir -p $(MIDIR) ;\
	fi
