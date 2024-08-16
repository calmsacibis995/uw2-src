#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/admbin/PS/FILTER/Makefile	1.2"
#ident	"$Header: $"

include $(CMDRULES)

BINDIR=$(ROOT)/$(MACH)/usr/vmsys/admin/PS/FILTER
BINARY=chgflt
SHELLS=add chkdel chkmode delete mitem mode name option pname ptype seek
PRODUCTS=$(BINARY) $(SHELLS)
GRP=bin
OWN=bin

all: $(PRODUCTS)
	@echo "***** Making VMSYS/admin/PS/FILTER"

$(BINARY):
	$(CC) $(CFLAGS) $(DEFLIST) $(@).c -o $(@) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

$(SHELLS):
	-for f in $(SHELLS) ; \
	do \
		cp $$f.sh $$f ; \
	done

install: all
	-[ -d $(BINDIR) ] || mkdir -p $(BINDIR)
	-for f in $(PRODUCTS) ; \
	do \
		$(INS) -m 755 -g $(GRP) -u $(OWN) -f $(BINDIR) $$f ; \
	done

clean:
	@echo "***** Cleaning VMSYS/admin/PS/FILTER"
	rm -f *.o 

clobber: clean
	@echo "***** Clobbering VMSYS/admin/PS/FILTER"
	rm -f $(PRODUCTS) lint.out

size:
	$(SIZE) $(BINARY)

strip:
	$(STRIP) $(BINARY)

lintit:
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(BINARY).c $(LINTLIBS) >> lint.out

