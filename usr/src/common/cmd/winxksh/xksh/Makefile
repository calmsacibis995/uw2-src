#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)winxksh:xksh/Makefile	1.21"

XKSTDVARS = stdvars
include $(XKSTDVARS)
include $(CMDRULES)

LIBDL = -ldl

MALLOCOBJ =

WINDIR = $(ROOT)/usr/src/$(WORK)/cmd/winxksh/libwin

KSHDIR = $(ROOT)/usr/src/$(WORK)/cmd/ksh

KSHSH = $(KSHDIR)/sh

KSHSHLIB = $(KSHDIR)/shlib

KSHINC = $(KSHDIR)/include

TARGET = xksh

XKSH_OBJS = extra.o newparse.o docall.o genlib.o define.o struct.o \
	symbolic.o misc.o xkstak.o cvar.o basetbl.o special.o $(MOREOBJS)

NORMKSH_OBJS = $(KSHSH)/arith.o $(KSHSH)/args.o $(KSHSH)/builtin.o \
    $(KSHSH)/cmd.o $(KSHSH)/ctype.o $(KSHSH)/outmsg.o $(KSHSH)/defs.o \
    $(KSHSH)/edit.o $(KSHSH)/echo.o $(KSHSH)/error.o $(KSHSH)/expand.o \
    $(KSHSH)/fault.o $(KSHSH)/history.o $(KSHSH)/io.o $(KSHSH)/jobs.o \
    $(KSHSH)/macro.o $(KSHSH)/print.o $(KSHSH)/service.o $(KSHSH)/string.o \
    $(KSHSH)/stak.o $(KSHSH)/test.o $(KSHSH)/word.o $(KSHSH)/xec.o \
    $(KSHSH)/emacs.o $(KSHSH)/vi.o $(KSHSHLIB)/libsh.a

RECOMPILE_KSH_OBJS = msg.o main.o name.o

RECOMPILE_KSH_CODE = $(RECOMPILE_KSH_OBJS:.o=.c)

KSH_OBJS = $(RECOMPILE_KSH_OBJS) $(NORMKSH_OBJS)

UFLAGS = $(SUBCFLAGS)

ULDFLAGS = $(SUBLDFLAGS)

CFLAGS = -I. -DKSHELL -I$(KSHINC) $(UFLAGS) $(DYNFLAG) $(STATICFLAG)

KSH_SUBCFL = -O

.PRECIOUS: xksh

all: readonly all$(TYPE) winxksh

include objmk.$(TYPE)

kshall.o: $(KSH_OBJS)
	$(LD) $(ULDFLAGS) -r -o $@ $(KSH_OBJS)

$(NORMKSH_OBJS):
	: echo $@ is out-of-date
	ls -l $@

$(XKSH_OBJS): $(@:.o=.c) $(KSHINC)/sh_config.h xksh.h

readonly:	readonly.sh
	@-rm -f readonly
	@cp readonly.sh readonly; chmod 555 readonly

gensym: gensym.sh
	@-rm -f gensym
	@cp gensym.sh gensym; chmod 555 gensym

gensym.o: gensym.c readonly
	./readonly -c $(DCONST) $(CFLAGS) gensym.c

$(RECOMPILE_KSH_CODE): $(KSHSH)/$$(@F)
	rm -f $@; cp $? $@

$(RECOMPILE_KSH_OBJS): $(RECOMPILE_KSH_CODE) extra.h
	@rm -f $@
	@$(MAKE) LOCALDEF='-DKSHELL -DXKSH -DMULTIBYTE -D_locale_' KSHINC="$(KSHINC)" -f $(KSHSH)/sh.mk $@

dowinxksh: xksh
	@> dowinxksh
	@$(MAKE) TYPE=$(TYPE) MOREOBJS=$(WINDIR)/allwin.o MORESYM=force.over TARGET=winxksh

winxksh: dowinxksh

install:
	$(INS) -f $(USRBIN) -m $(BINMODE) -u $(OWN) -g $(GRP) xksh
	$(INS) -f $(USRBIN) -m $(BINMODE) -u $(OWN) -g $(GRP) winxksh

clean: 
	rm -f \
	$(RECOMPILE_KSH_CODE) \
	*.list \
	*.o \
	*.pre \
	dowinxksh \
	gensym \
	gensym.args \
	gensym.c \
	gensym.o \
	libforce.so \
	nuwinxksh \
	nuxksh \
	winxksh.o \
	xksh.o

clobber : clean
	rm -f xksh winxksh readonly
