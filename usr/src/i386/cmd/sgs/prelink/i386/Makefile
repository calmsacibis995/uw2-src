#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)prelink:i386/Makefile	1.6"

#
#	Prelinker demangler makefile
#
include $(CMDRULES)

LINTFLAGS=
LINK_MODE=

LDLIBS=

INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSLIB)

HFILES=$(COMDIR)/edg_prelink.h

SOURCES=$(COMDIR)/edg_prelink.c
OBJECTS=edg_prelink.o
PRODUCTS=prelink
PROBEFILE=$(COMDIR)/edg_prelink.c

DEFLIST = -D__SYSV__

# We have to specify the cplusfe directories this way so the we get 
# the right thing whether $(COMDIR) and $(CPUDIR) are specified as
# relative or absolute pathnames.
INCLIST=-I. -I$(COMDIR) -I$(CPUINC) -I$(COMINC) -I$(CPUDIR)/../../cplusfe/$(CPU) -I$(COMDIR)/../../cplusfe/common
CC_CMD=$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST)

all:
	if [ -f $(PROBEFILE) ]; \
	then \
		find $(PRODUCTS) \( ! -type f -o -links +1 \) \
		    -exec rm -f {} \; 2>/dev/null; \
		$(MAKE) $(PRODUCTS) $(MAKEARGS); \
	else \
		for file in $(PRODUCTS); \
		do \
			if [ ! -r $$file ]; \
			then \
				echo "ERROR: $$file is missing" 1>&2; \
				false; \
				break; \
			fi \
		done \
	fi 

$(PRODUCTS): 	$(OBJECTS) $(LDLIBS)
	$(CC) $(OBJECTS) $(LDLIBS) $(LINK_MODE) $(LIBSGS) -o $(PRODUCTS)

edg_prelink.o:	$(HFILES) $(COMDIR)/edg_prelink.c
	$(CC_CMD) -c $(COMDIR)/edg_prelink.c 

install : all
	$(MV) $(PRODUCTS) $(PRODUCTS).bak
	$(CP) $(PRODUCTS).bak $(PRODUCTS)
	$(STRIP) $(PRODUCTS)
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)$(PRODUCTS) $(PRODUCTS)
	$(MV) $(PRODUCTS).bak $(PRODUCTS)

lintit:	$(HFILES) $(SOURCES)
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $(SOURCES)

clean:
	-rm -f $(OBJECTS)

shrink clobber: clean
	-if [ -f $(PROBEFILE) ]; \
	then \
		rm -f $(PRODUCTS); \
	fi
