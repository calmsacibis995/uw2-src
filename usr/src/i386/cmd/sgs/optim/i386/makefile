#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)optim:i386/makefile	1.1.3.21"
#
#	i386 OPTIMIZER MAKEFILE
#
#
# Some optimizations by default will help 486 performance while hurting
# 386 performance.  To change the default setting (optimize for the 386),
# add -DI386_OPTS -UP5 to CFLAGS below.
#
include $(CMDRULES)
LDFLAGS=

LINTFLAGS=-Xa $(DEFLIST) $(INCLIST)

INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSLIB)

SOURCES=$(COMDIR)/inter.c $(COMDIR)/optim.c sched.c ebboptim.c \
	debug.c imull.c local.c optutil.c \
	peep.c regal.c w1opt.c w2opt.c w3opt.c w4opt.c w5opt.c w6opt.c \
	postpeep.c database.c loops.c fp_timings.c
OBJECTS=inter.o optim.o sched.o ebboptim.o \
	debug.o imull.o local.o optutil.o \
	peep.o regal.o w1opt.o w2opt.o w3opt.o w4opt.o w5opt.o w6opt.o \
	postpeep.o database.o loops.o fp_timings.o
HFILES=database.h debug.h defs.h fp_timings.h optutil.h regal.h sched.h \
	$(COMDIR)/optim.h

PRODUCTS=optim

UW11_PRODUCTS=optim_CC

INCLIST= -I. -I$(COMDIR) -I$(CPUINC) -I$(COMINC)  -I$(INC)
CC_CMD=$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST)
LD_CMD=$(CC) $(LDFLAGS)
LINT_CMD=$(LINT) $(LINTFLAGS)
CSCOPE_CMD=cscope

all:		$(PRODUCTS)

all_uw11:		$(UW11_PRODUCTS)

optim:		$(OBJECTS)
		$(LD_CMD) $(OBJECTS) $(LDLIBS) -o optim $(LIBSGS)

optim_CC:		$(OBJECTS)
		$(LD_CMD) $(OBJECTS) $(LDLIBS) -o optim_CC $(LIBSGS)

inter.o:	$(COMDIR)/optim.h defs.h $(COMDIR)/inter.c
		$(CC_CMD) $(COMDIR)/inter.c

optim.o:	$(COMDIR)/optim.h defs.h $(COMDIR)/optim.c
		$(CC_CMD) $(COMDIR)/optim.c

database.o:	defs.h database.h sched.h $(COMDIR)/optim.h database.c
		$(CC_CMD) database.c	

fp_timings.o: defs.h $(COMDIR)/optim.h database.h fp_timings.c fp_timings.h
		$(CC_CMD) fp_timings.c	

loops.o:	defs.h  fp_timings.h loops.c database.h sched.h
		$(CC_CMD) loops.c

debug.o:	defs.h debug.h regal.h debug.c
		$(CC_CMD) debug.c

ebboptim.o:	defs.h sched.h database.h $(COMDIR)/optim.h optutil.h ebboptim.c
		$(CC_CMD) ebboptim.c

imull.o:	$(COMDIR)/optim.h defs.h optutil.h imull.c
		$(CC_CMD) imull.c

local.o:	$(COMDIR)/optim.h defs.h optutil.h $(CPUINC)/sgs.h debug.h \
		$(CPUINC)/paths.h regal.h $(COMINC)/storclass.h local.c
		$(CC_CMD) local.c

optutil.o:	$(COMDIR)/optim.h defs.h optutil.h sched.h database.h debug.h \
			regal.h optutil.c
		$(CC_CMD) optutil.c

peep.o:		$(COMDIR)/optim.h defs.h optutil.h database.h regal.h peep.c
		$(CC_CMD) peep.c

postpeep.o:	defs.h sched.h $(COMDIR)/optim.h database.h optutil.h regal.h \
			postpeep.c
		$(CC_CMD) postpeep.c

regal.o:	$(COMDIR)/optim.h defs.h optutil.h regal.h debug.h \
		$(COMINC)/storclass.h regal.c
		$(CC_CMD) regal.c

sched.o:	defs.h sched.h optutil.h $(COMDIR)/optim.h database.h sched.c
		$(CC_CMD) sched.c

w1opt.o:	$(COMDIR)/optim.h defs.h optutil.h database.h sched.h w1opt.c
		$(CC_CMD) w1opt.c

w2opt.o:	$(COMDIR)/optim.h defs.h optutil.h sched.h database.h w2opt.c
		$(CC_CMD) w2opt.c

w3opt.o:	$(COMDIR)/optim.h defs.h optutil.h sched.h database.h w3opt.c
		$(CC_CMD) w3opt.c

w4opt.o:	$(COMDIR)/optim.h defs.h optutil.h w4opt.c
		$(CC_CMD) w4opt.c

w5opt.o:	$(COMDIR)/optim.h defs.h optutil.h w5opt.c
		$(CC_CMD) w5opt.c

w6opt.o:	$(COMDIR)/optim.h defs.h optutil.h w6opt.c
		$(CC_CMD) w6opt.c


install:	$(INSDIR)/$(SGS)optim

install_all_uw11:	$(INSDIR)/$(SGS)optim_CC

$(INSDIR)/$(SGS)optim:	all
		cp optim optim.bak
		-$(STRIP) optim
		/bin/sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)optim optim
		mv optim.bak optim

$(INSDIR)/$(SGS)optim_CC:	all_uw11
		cp optim_CC optim_CC.bak
		-$(STRIP) optim_CC
		/bin/sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)optim_CC optim_CC
		mv optim_CC.bak optim_CC

save:		$(INSDIR)/$(SGS)optim
		-rm -f $(INSDIR)/$(SGS)optim.back
		cp $(INSDIR)/$(SGS)optim $(INSDIR)/$(SGS)optim.back

uninstall:	$(INSDIR)/$(SGS)optim.back
		-rm -f $(INSDIR)/$(SGS)optim
		cp $(INSDIR)/$(SGS)optim.back $(INSDIR)/optim

save_uw11:	$(INSDIR)/$(SGS)optim_CC
		-rm -f $(INSDIR)/$(SGS)optim_CC.back
		cp $(INSDIR)/$(SGS)optim_CC $(INSDIR)/$(SGS)optim_CC.back

uninstall_uw11:	$(INSDIR)/$(SGS)optim_CC.back
		-rm -f $(INSDIR)/$(SGS)optim_CC
		cp $(INSDIR)/$(SGS)optim_CC.back $(INSDIR)/optim_CC

lintit:		$(SOURCES)
		$(LINT_CMD) $(SOURCES)

x.shar:		$(HFILES) $(SOURCES) makefile
			shar $(HFILES) $(SOURCES) makefile > x.shar

cscope.out: $(HFILES) $(SOURCES)
		$(CSCOPE_CMD) $(HFILES) $(SOURCES)

clean:
		-rm -f $(OBJECTS)

shrink:		clobber

clobber:	clean
		-rm -f $(PRODUCTS) $(UW11_PRODUCTS)

new:		clean  all
