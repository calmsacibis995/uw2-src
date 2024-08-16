#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sgs-cmd:i386/makefile	1.1.1.19"

include $(CMDRULES)

INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSBIN)

HFILES = $(CPUDIR)/machdep.h $(COMDIR)/cc.h $(CPUINC)/paths.h $(CPUINC)/sgs.h

SOURCES = $(COMDIR)/cc.c $(CPUDIR)/machdep.c

CC_OBJECTS = CC.d/cc.o CC.d/machdep.o
cc_OBJECTS = cc.d/cc.o cc.d/machdep.o

OBJECTS = $(CC_OBJECTS) $(cc_OBJECTS)

PRODUCTS = cc CC

CC_DEFLIST = -D$(ARCH) -D$(CPU) -DCCflag=1 \
             -DAUTOMATIC_TEMPLATE_INSTANTIATION=1 \
             -DDEFAULT_AUTOMATIC_INSTANTIATION_MODE=1

cc_DEFLIST = -D$(ARCH) -D$(CPU) -DCCflag=0

INCLIST=-I. -I$(COMDIR) -I$(CPUINC) -I$(INC) -I$(COMINC)

CC_CMD = $(CC) $(CFLAGS) $(CC_DEFLIST) $(INCLIST)
cc_CMD = $(CC) $(CFLAGS) $(cc_DEFLIST) $(INCLIST)

.MUTEX: all $(INSDIR)/$(SGS)cc $(INSDIR)/$(SGS)CC

.MUTEX: cc.d CC.d $(PRODUCTS)

all: cc.d CC.d	$(PRODUCTS)
	
cc : $(cc_OBJECTS)
	$(CC) $(cc_OBJECTS) $(LINK_MODE) $(LIBSGS) -o $(@)

CC : $(CC_OBJECTS)
	$(CC) $(CC_OBJECTS) $(LINK_MODE) $(LIBSGS) -o $(@)

cc.d:
	mkdir $(@)

CC.d:
	mkdir $(@)


.MUTEX:	all do_install_cc do_install_CC

install : all do_install_cc do_install_CC

do_install_cc : $(INSDIR)/$(SGS)cc

do_install_CC : $(INSDIR)/$(SGS)CC

$(INSDIR)/$(SGS)cc : cc
	$(MV) cc cc.bak
	$(CP) cc.bak cc
	$(STRIP) cc
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(@) cc
	$(MV) cc.bak cc

$(INSDIR)/$(SGS)CC : CC
	$(MV) CC CC.bak
	$(CP) CC.bak CC
	$(STRIP) CC
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(@) CC
	$(MV) CC.bak CC

lintit:	$(HFILES) $(SOURCES)
	$(LINT) $(LINTFLAGS) $(CC_DEFLIST) $(INCLIST) $(SOURCES)


# all_uw11 and install_all_uw11 are special targets
# for building CC only for UnixWare 1.1

all_uw11 :
	$(MAKE) _all_uw11 CC_DEFLIST="$(CC_DEFLIST) -DUNIXWARE_1_1 -DN_C0=\\\"acomp_CC\\\" -DN_OPTIM=\\\"optim_CC\\\"" CC="$(CC)" CFLAGS="$(CFLAGS)" INCLIST="$(INCLIST)"

_all_uw11 : CC.d CC

install_all_uw11 : all_uw11 do_install_CC


clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(PRODUCTS)

cc.d/cc.o: $(HFILES) $(COMDIR)/cc.c
	$(cc_CMD) -c $(COMDIR)/cc.c && $(MV) $(@F) $(@)

cc.d/machdep.o : $(HFILES) $(CPUDIR)/machdep.c
	$(cc_CMD) -c $(CPUDIR)/machdep.c && $(MV) $(@F) $(@)

CC.d/cc.o : $(HFILES) $(COMDIR)/cc.c
	$(CC_CMD) -c $(COMDIR)/cc.c && $(MV) $(@F) $(@)

CC.d/machdep.o : $(HFILES) $(CPUDIR)/machdep.c
	$(CC_CMD) -c $(CPUDIR)/machdep.c && $(MV) $(@F) $(@)

